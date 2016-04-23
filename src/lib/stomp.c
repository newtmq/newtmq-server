#include <kazusa/stomp.h>
#include <kazusa/common.h>
#include <kazusa/signal.h>
#include <kazusa/logger.h>

#include <kazusa/stomp_management_worker.h>
#include <kazusa/stomp_message_worker.h>

#include <assert.h>

struct stomp_frame_info {
  char *name;
  int len;
};
static struct stomp_frame_info finfo_arr[] = {
  {"ACK",         3},
  {"SEND",        4},
  {"SUBSCRIBE",   9},
  {"CONNECT",     7},
  {"STOMP",       5},
  {"BEGIN",       5},
  {"COMMIT",      6},
  {"ABORT",       5},
  {"NACK",        4},
  {"UNSUBSCRIBE", 11},
  {"DISCONNECT",  10},
  {0},
};

frame_bucket_t stomp_frame_bucket;

frame_t *alloc_frame() {
  frame_t *ret;

  debug("(alloc_frame)");

  ret = (frame_t *)malloc(sizeof(frame_t));
  if(ret == NULL) {
    return NULL;
  }

  /* Initialize frame_t object */
  memset(ret->name, 0, FNAME_LEN);

  INIT_LIST_HEAD(&ret->h_attrs);
  INIT_LIST_HEAD(&ret->h_data);

  ret->sock = 0;
  ret->cinfo = NULL;
  ret->status = STATUS_BORN;

  return ret;
}

void free_frame(frame_t *frame) {
  linedata_t *data;

  /* delete header */
  if(! list_empty(&frame->h_attrs)) {
    list_for_each_entry(data, &frame->h_attrs, l_frame) {
      free(data);
    }
  }

  /* delete body */
  if(! list_empty(&frame->h_attrs)) {
    list_for_each_entry(data, &frame->h_data, l_frame) {
      free(data);
    }
  }

  pthread_mutex_lock(&stomp_frame_bucket.mutex);
  {
    if(frame->l_bucket.next != NULL && frame->l_bucket.prev != NULL) {
      list_del(&frame->l_bucket);
    }
  }
  pthread_mutex_unlock(&stomp_frame_bucket.mutex);

  free(frame);
}

char *ssplit(char *str, char *end) {
  char *curr = str;

  assert(curr != NULL);

  while(! (*curr == '\n' || *curr == '\0')) {
    if(curr >= end) {
      char debug_buf[LD_MAX] = {0};

      memcpy(debug_buf, str, (int)(end - str));
      warn("[ssplit] rest str: %s", debug_buf);

      return NULL;
    }

    curr++;
  }

  return curr;
}

static void frame_setname(char *data, int len, frame_t *frame) {
  int namelen = len;

  debug("(frame_setname) name: %s [%d]", data, len);

  if(namelen > FNAME_LEN) {
    namelen = FNAME_LEN;
  }
  memcpy(frame->name, data, namelen);

  CLR(frame);
  SET(frame, STATUS_INPUT_NAME);
}

static int frame_setdata(char *data, int len, struct list_head *head) {
  linedata_t *attr;
  int attrlen = len;

  if(attrlen > LD_MAX) {
    attrlen = LD_MAX;
  }

  attr = (linedata_t *)malloc(sizeof(linedata_t));
  if(attr == NULL) {
    warn("failed to allocate linedata_t");
    return RET_ERROR;
  }

  memset(attr, 0, sizeof(linedata_t));
  memcpy(attr->data, data, attrlen);
  INIT_LIST_HEAD(&attr->l_frame);

  list_add_tail(&attr->l_frame, head);

  debug("(frame_setdata) attr-data > %s [%d]", attr->data, attrlen);

  return RET_SUCCESS;
}

static int cleanup(void *data) {
  frame_t *frame;
  frame_t *h = NULL;

  pthread_mutex_lock(&stomp_frame_bucket.mutex);
  { /* thread safe */
    list_for_each_entry_safe(frame, h, &stomp_frame_bucket.h_frame, l_bucket) {
      free_frame(frame);
    }
  }
  pthread_mutex_unlock(&stomp_frame_bucket.mutex);

  return RET_SUCCESS;
}

static void frame_finish(frame_t *frame) {
  // last processing for current frame_t object
  CLR(frame);
  SET(frame, STATUS_IN_BUCKET);

  debug("(frame_finish) %s", frame->name);

  pthread_mutex_lock(&stomp_frame_bucket.mutex);
  { /* thread safe */
    list_add_tail(&frame->l_bucket, &stomp_frame_bucket.h_frame);
  }
  pthread_mutex_unlock(&stomp_frame_bucket.mutex);
}

static int frame_update(char *line, int len, frame_t *frame) {

  debug("(frame_update) >> %s [%d]", line, len);

  if(IS_BL(line)) {
    if(GET(frame, STATUS_INPUT_NAME) || GET(frame, STATUS_INPUT_HEADER) || GET(frame, STATUS_INPUT_BODY) ) {
      frame_finish(frame);

      return 1;
    }
  } else if(IS_NL(line)) {
    /* The case of blankline is inputed */
    if(GET(frame, STATUS_INPUT_HEADER)) {
      if(strcmp(frame->name, "SEND") == 0 || strcmp(frame->name, "MESSAGE") == 0) {
        CLR(frame);
        SET(frame, STATUS_INPUT_BODY);
      } else {
        frame_finish(frame);

        return 1;
      }
    } else if(GET(frame, STATUS_INPUT_BODY)) {
      frame_finish(frame);

      return 1;
    }
  } else {
    if(GET(frame, STATUS_BORN)) {
      frame_setname(line, len, frame);
    } else if(GET(frame, STATUS_INPUT_NAME)) {
      /* In some case, a blank line may inserted between name and headers */
      DEL(frame, STATUS_INPUT_NAME);
      SET(frame, STATUS_INPUT_HEADER);

      frame_setdata(line, len, &frame->h_attrs);
    } else if(GET(frame, STATUS_INPUT_HEADER)) {
      frame_setdata(line, len, &frame->h_attrs);
    } else if(GET(frame, STATUS_INPUT_BODY)) {
      debug("(frame_update) (setdata) >> %s [%d]", line, len);
      frame_setdata(line, len, &frame->h_data);
    }
  }

  return 0;
}

int stomp_init() {
  INIT_LIST_HEAD(&stomp_frame_bucket.h_frame);
  pthread_mutex_init(&stomp_frame_bucket.mutex, NULL);

  set_signal_handler(cleanup, NULL);

  stomp_message_worker_init();

  return RET_SUCCESS;
}

stomp_conninfo_t *stomp_conn_init() {
  stomp_conninfo_t *ret;

  ret = (stomp_conninfo_t *)malloc(sizeof(stomp_conninfo_t));
  if(ret != NULL) {
    memset(ret, 0, sizeof(stomp_conninfo_t));

    /* intiialize params */
    ret->status = 0;
    ret->remained_data = NULL;
    SET(ret, STATE_INIT);
    memset(ret->line_buf, 0, LD_MAX);
  }

  return ret;
}

int stomp_conn_finish(void *data) {
  stomp_conninfo_t *cinfo = (stomp_conninfo_t *)data;
  int ret = RET_ERROR;

  if(cinfo != NULL) {
    frame_t *frame = cinfo->frame;

    if(frame != NULL && (GET(frame, STATUS_INPUT_BODY) || GET(frame, STATUS_INPUT_NAME))) {
      frame_finish(frame);

      ret = RET_SUCCESS;
    }
  }

  return ret;
}

static int do_stomp_recv_data(char *line, int len, int sock, void *_cinfo) {
  stomp_conninfo_t *cinfo = (stomp_conninfo_t *)_cinfo;

  if(cinfo == NULL || (cinfo->frame == NULL && len == 0)) {
    return RET_ERROR;
  }

  debug("(do_stomp_recv_data) %s [%d]", line, len);

  if(IS_BL(line) || IS_NL(line)) {
    if(frame_update(line, len, cinfo->frame) > 0) {
      /* In this case, some problem is occurred */
      cinfo->frame = NULL;
    }

    return RET_SUCCESS;
  }

  if(cinfo->frame != NULL) {
    int i;
    struct stomp_frame_info *finfo;

    for(i=0; finfo=&finfo_arr[i], finfo!=NULL; i++) {
      if(len < finfo->len || finfo->name == NULL) {
        break;
      }

      if(strncmp(line, finfo->name, finfo->len) == 0) {
        debug("(stomp_recv_data) <matched %s [%d]>", finfo->name, finfo->len);
        frame_finish(cinfo->frame);
        cinfo->frame = NULL;
        break;
      }
    }
  }

  if(cinfo->frame == NULL) {
    frame_t *frame = alloc_frame(sock);
    if(frame == NULL) {
      perror("[stomp_recv_data] failed to allocate memory");
      return RET_ERROR;
    }

    /* initialize frame params */
    frame->sock = sock;
    frame->cinfo = cinfo;

    /* to reference this values over the 'stomp_recv_data' calling */
    cinfo->frame = frame;
  }
  
  if(frame_update(line, len, cinfo->frame) > 0) {
    /* In this case, some problem is occurred */
    cinfo->frame = NULL;
  }

  return RET_SUCCESS;
}

int stomp_recv_data(char *recv_data, int len, int sock, void *_cinfo) {
  stomp_conninfo_t *cinfo = (stomp_conninfo_t *)_cinfo;
  char *curr, *next, *end;

  debug("(stomp_recv_data) %s [%d]", recv_data, len);

  curr = recv_data;
  end = (recv_data + len);
  while(curr < end) {
    int line_len;
    int line_prefix = 0;

    if(cinfo->remained_data != NULL) {
      debug("(stomp_recv_data) remained_data: %s [%d]", cinfo->remained_data, cinfo->remained_size);

      memcpy(cinfo->line_buf, cinfo->remained_data, cinfo->remained_size);
      free(cinfo->remained_data);

      cinfo->remained_data = NULL;
      line_prefix = cinfo->remained_size;
    }

    next = ssplit(curr, end);
    if(next == NULL) {
      int remained_size = (int)(end - curr);
      char *remained_data = (char *)malloc(remained_size);

      if(remained_data != NULL) {
        memcpy(remained_data, curr, remained_size);

        cinfo->remained_data = remained_data;
        cinfo->remained_size = remained_size;
      }

      break;
    }

    line_len = (int)(next - curr);
    if(line_len > LD_MAX) {
      line_len = LD_MAX;
    }

    if(line_len > 0) {
      memcpy((cinfo->line_buf + line_prefix), curr, line_len);
      cinfo->line_buf[line_len + line_prefix] = '\0';
    } else {
      cinfo->line_buf[0] = *curr;
      cinfo->line_buf[1] = '\0';
    }

    do_stomp_recv_data(cinfo->line_buf, line_len + line_prefix, sock, _cinfo);

    curr = next + 1;
  }

  return RET_SUCCESS;
}

frame_t *get_frame_from_bucket() {
  frame_t *frame = NULL;

  pthread_mutex_lock(&stomp_frame_bucket.mutex);
  { /* thread safe */
    if(! list_empty(&stomp_frame_bucket.h_frame)) {
      frame = list_first_entry(&stomp_frame_bucket.h_frame, frame_t, l_bucket);
      list_del(&frame->l_bucket);
    }
  }
  pthread_mutex_unlock(&stomp_frame_bucket.mutex);

  return frame;
}

void stomp_send_error(int sock, char *body) {
  int i;
  char *msg[] = {
    "ERROR\n",
    "\n",
    body,
    NULL,
  };

  for(i=0; msg[i] != NULL; i++) {
    send_msg(sock, msg[i]);
  }
  send_msg(sock, NULL);
}
