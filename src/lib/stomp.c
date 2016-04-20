#include <kazusa/stomp.h>
#include <kazusa/common.h>
#include <kazusa/signal.h>
#include <kazusa/logger.h>

#include <kazusa/stomp_management_worker.h>
#include <kazusa/stomp_message_worker.h>

frame_bucket_t stomp_frame_bucket;

frame_t *alloc_frame() {
  frame_t *ret;

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
  list_for_each_entry(data, &frame->h_attrs, l_frame) {
    free(data);
  }

  /* delete body */
  list_for_each_entry(data, &frame->h_data, l_frame) {
    free(data);
  }

  if(frame->l_bucket.next != NULL && frame->l_bucket.prev != NULL) {
    list_del(&frame->l_bucket);
  }

  free(frame);
}

static void frame_setname(char *data, int len, frame_t *frame) {
  int namelen = len;

  logger(LOG_DEBUG, "(frame_setname) name: %s [%d]", data, len);

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
    logger(LOG_WARN, "failed to allocate linedata_t");
    return RET_ERROR;
  }
  memcpy(attr->data, data, attrlen);
  INIT_LIST_HEAD(&attr->l_frame);

  list_add_tail(&attr->l_frame, head);

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

static void frame_create_finish(frame_t *frame) {
  // last processing for current frame_t object
  CLR(frame);
  SET(frame, STATUS_IN_BUCKET);

  logger(LOG_DEBUG, "(frame_create_finish) %s", frame->name);

  pthread_mutex_lock(&stomp_frame_bucket.mutex);
  { /* thread safe */
    list_add_tail(&frame->l_bucket, &stomp_frame_bucket.h_frame);
  }
  pthread_mutex_unlock(&stomp_frame_bucket.mutex);
}

static int making_frame(char *recv_data, int len, frame_t *frame) {
  char *p, *line;

  for(p=recv_data; line=strtok_single(p, "\n"); p += strlen(line) + 1) {
    int attrlen = strlen(line);

    logger(LOG_DEBUG, "(making_frame) >> %s", line);

    if(not_bl(line)) {
      if(GET(frame, STATUS_BORN)) {
        frame_setname(line, attrlen, frame);
      } else if(GET(frame, STATUS_INPUT_NAME)) {
        /* In some case, a blank line may inserted between name and headers */
        DEL(frame, STATUS_INPUT_NAME);
        SET(frame, STATUS_INPUT_HEADER);

        frame_setdata(line, attrlen, &frame->h_attrs);
      } else if(GET(frame, STATUS_INPUT_HEADER)) {
        frame_setdata(line, attrlen, &frame->h_attrs);
      } else if(GET(frame, STATUS_INPUT_BODY)) {
        frame_setdata(line, attrlen, &frame->h_data);
      }
    } else {
      /* The case of blankline is inputed */
      if(GET(frame, STATUS_BORN)) {
        stomp_send_error(frame->sock, "syntax error");

        free_frame(frame);

        return 1;
      } else if(GET(frame, STATUS_INPUT_BODY)) {
        frame_create_finish(frame);

        return 1;
      } else if(GET(frame, STATUS_INPUT_NAME)) {
        // ignore
        continue;
      } else if(GET(frame, STATUS_INPUT_HEADER)) {
        if(strcmp(frame->name, "SEND") == 0 || strcmp(frame->name, "MESSAGE") == 0) {
          CLR(frame);
          SET(frame, STATUS_INPUT_BODY);
        } else {
          frame_create_finish(frame);

          return 1;
        }
      }
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
    SET(ret, STATE_INIT);
  }

  return ret;
}

int stomp_conn_finish(void *data) {
  stomp_conninfo_t *cinfo = (stomp_conninfo_t *)data;
  int ret = RET_ERROR;

  if(cinfo != NULL) {
    frame_t *frame = cinfo->frame;

    if(frame != NULL && (GET(frame, STATUS_INPUT_BODY) || GET(frame, STATUS_INPUT_NAME))) {
      frame_create_finish(frame);

      ret = RET_SUCCESS;
    }
  }

  return ret;
}

int stomp_recv_data(char *recv_data, int len, int sock, void *_cinfo) {
  stomp_conninfo_t *cinfo = (stomp_conninfo_t *)_cinfo;

  if(cinfo == NULL || (cinfo->frame == NULL && len == 0)) {
    return RET_ERROR;
  }

  logger(LOG_DEBUG, "(stomp_recv_data) %s [%d]", recv_data, len);

  if(cinfo->frame == NULL) {
    /* ignore no meaning blank input */
    if(! not_bl(recv_data)) {
      return RET_SUCCESS;
    }

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

  if(len == 0) {
    /* Stop making frame in any way. This means user send '^@' which means EOL */
    frame_create_finish(cinfo->frame);

    /* Because target frame is stored in frame bucket */
    cinfo->frame = NULL;
  }
  
  if(making_frame(recv_data, len, cinfo->frame) > 0) {
    /* In this case, some problem is occurred */
    cinfo->frame = NULL;
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
