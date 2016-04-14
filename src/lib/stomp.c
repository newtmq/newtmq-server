#include <kazusa/stomp.h>
#include <kazusa/common.h>
#include <kazusa/signal.h>

#include <string.h>

typedef struct stomp_handler_t {
  char *name;
  frame_t *(*handler)(frame_t *);
} stomp_handler_t;

/* This object is accessed globally */
frame_bucket_t stomp_frame_bucket;

static stomp_handler_t stomp_handlers[] = {
  {"SEND", NULL}, // not implemented yet
  {"SUBSCRIBE", NULL}, // not implemented yet
  {"CONNECT", handler_stomp_connect},
  {"DISCONNECT", NULL}, // not implemented yet
  {"UNSUBSCRIBE", NULL}, // not implemented yet
  {"BEGIN", NULL}, // not implemented yet
  {"COMMIT", NULL}, // not implemented yet
  {"ABORT", NULL}, // not implemented yet
  {"ACK", NULL}, // not implemented yet
  {"NACK", NULL}, // not implemented yet
  {0},
};

static char *strtok_single(char * str, char const * delims) {
  static char  * src = NULL;
  char  *  p,  * ret = 0;

  if(str != NULL){
    src = str;
  }

  if(src == NULL){
    return NULL;
  }

  if((p = strpbrk (src, delims)) != NULL){
    *p  = 0;
    ret = src;
    src = ++p;
  }else if (*src){
    ret = src;
    src = NULL;
  }

  return ret;
}

static frame_t *alloc_frame() {
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

static void free_frame(frame_t *frame) {
  linedata_t *data;

  list_for_each_entry(data, &frame->h_attrs, l_frame) {
    free(data);
  }
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

  if(namelen > FNAME_LEN) {
    namelen = FNAME_LEN;
  }
  memcpy(frame->name, data, len);

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
    printf("[warning] failed to allocate linedata_t\n");
    return RET_ERROR;
  }
  memset(attr->data, 0, LD_MAX);
  memcpy(attr->data, data, len);

  list_add_tail(&attr->l_frame, head);

  return RET_SUCCESS;
}

static int cleanup(void *data) {
  frame_t *frame;
  frame_t *h = NULL;

  printf("[debug] (stomp/cleanup)\n");

  pthread_mutex_lock(&stomp_frame_bucket.mutex);
  { /* thread safe */
    list_for_each_entry_safe(frame, h, &stomp_frame_bucket.h_frame, l_bucket) {
      free_frame(frame);
    }
  }
  pthread_mutex_unlock(&stomp_frame_bucket.mutex);

  printf("[debug] (stomp/cleanup) finished\n");

  return RET_SUCCESS;
}

static void frame_create_finish(frame_t *frame) {
  // last processing for current frame_t object
  CLR(frame);
  SET(frame, STATUS_IN_BUCKET);

  printf("[debug] (frame_crate_finish) frame: %p\n", frame);

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

    printf("[debug] (making_frame) %s [%d]\n", line, attrlen);

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

int stomp_recv_data(char *recv_data, int len, int sock, void *_cinfo) {
  stomp_conninfo_t *cinfo = (stomp_conninfo_t *)_cinfo;

  if(cinfo == NULL) {
    return RET_ERROR;
  }

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

static frame_t *get_frame_from_bucket() {
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

int iterate_header(struct list_head *h_header, stomp_header_handler_t *handlers, void *data) {
  linedata_t *line;

  list_for_each_entry(line, h_header, l_frame) {
    stomp_header_handler_t *h;
    int i;

    for(i=0; h=&handlers[i], h->name!=NULL; i++) {
      printf("[debug] (iterate_header) |%s / %s|\n", line->data, h->name);
      if(strncmp(line->data, h->name, strlen(h->name)) == 0) {
        int ret = (*h->handler)((line->data + strlen(h->name)), data);
        if(ret == RET_ERROR) {
          return RET_ERROR;
        }
      }
    }
  }

  return RET_SUCCESS;
}

static int handle_frame(frame_t *frame) {
  int i;
  stomp_handler_t *h;

  for(i=0; h=&stomp_handlers[i], h->name!=NULL; i++) {
    if(strncmp(frame->name, h->name, strlen(h->name)) == 0) {
      (*h->handler)(frame);
      break;
    }
  }
}

void *stomp_management_worker(void *data) {
  frame_t *frame;

  while(1) {
    frame = get_frame_from_bucket();
    if(frame != NULL) {
      printf("[debug] (stomp_management_worker) %p\n", frame);

      handle_frame(frame);

      free_frame(frame);
    }
  }

  return NULL;
}

void stomp_send_error(int sock, char *body) {
  char *msg[] = {
    "ERROR\n",
    "\n",
    body,
    NULL,
  };

  send_msg(sock, msg);
}
