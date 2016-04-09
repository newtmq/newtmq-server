#include <kazusa/stomp.h>
#include <kazusa/common.h>
#include <kazusa/signal.h>

/* This object is accessed globally */
frame_bucket_t stomp_frame_bucket;

static frame_t *alloc_frame(int sock) {
  frame_t *ret;

  ret = (frame_t *)malloc(sizeof(frame_t));
  if(ret == NULL) {
    return NULL;
  }

  /* Initialize frame_t object */
  memset(ret->name, 0, FNAME_LEN);

  INIT_LIST_HEAD(&ret->h_attrs);
  INIT_LIST_HEAD(&ret->h_data);

  ret->sock = sock;
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
  list_del(&frame->l_bucket);

  free(frame);
}

static void frame_setname(char *data, int len, frame_t *frame) {
  int namelen = len;

  if(namelen > FNAME_LEN) {
    namelen = FNAME_LEN;
  }
  memcpy(frame->name, data, len);

  CLR_STATUS(frame);
  SET_STATUS(frame, STATUS_INPUT_HEADER);
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
  frame_t *frame, *h;

  pthread_mutex_lock(&stomp_frame_bucket.mutex);
  { /* thread safe */
    list_for_each_entry_safe(frame, h, &stomp_frame_bucket.h_frame, l_bucket) {
      free_frame(frame);
    }
  }
  pthread_mutex_unlock(&stomp_frame_bucket.mutex);

  return RET_SUCCESS;
}

static void frame_creating(char *recv_data, int len, frame_t *frame) {
  char *line, *pointer;

  for(pointer = recv_data; line = strtok(pointer, "\n"); pointer += strlen(line) + 1) {
    int attrlen = strlen(line);

    if(GET_STATUS(frame, STATUS_BORN)) {
      frame_setname(line, attrlen, frame);
    } else if(GET_STATUS(frame, STATUS_INPUT_HEADER)) {
      frame_setdata(line, attrlen, &frame->h_attrs);
    } else if(GET_STATUS(frame, STATUS_INPUT_BODY)) {
      frame_setdata(line, attrlen, &frame->h_data);
    }
  }
}

static void frame_create_finish(frame_t *frame) {
  // last processing for current frame_t object
  CLR_STATUS(frame);
  SET_STATUS(frame, STATUS_IN_BUCKET);

  pthread_mutex_lock(&stomp_frame_bucket.mutex);
  { /* thread safe */
    list_add_tail(&frame->l_bucket, &stomp_frame_bucket.h_frame);
  }
  pthread_mutex_unlock(&stomp_frame_bucket.mutex);
}

int stomp_init() {
  INIT_LIST_HEAD(&stomp_frame_bucket.h_frame);
  pthread_mutex_init(&stomp_frame_bucket.mutex, NULL);

  set_signal_handler(cleanup, NULL);

  return RET_SUCCESS;
}

int stomp_recv_data(char *recv_data, int len, int sock, void **cache) {
  frame_t *frame = (frame_t *)*cache;

  if(frame == NULL) {
    frame = alloc_frame(sock);
    if(frame == NULL) {
      perror("[stomp_recv_data] failed to allocate memory");
      return RET_ERROR;
    }

    /* set a frame to take over this data */
    *cache = frame;
  }

  if(len == 0) {
    // the case when use send '^@' which means EOL.
    frame_create_finish(frame);
  } else if(len == 1 && *recv_data == '\n') {
    CLR_STATUS(frame);
    SET_STATUS(frame, STATUS_INPUT_BODY);
  } else {
    frame_creating(recv_data, len, frame);
  }

  return RET_SUCCESS;
}
