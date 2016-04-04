#include <kazusa/stomp.h>
#include <kazusa/common.h>

/* This object is accessed globally */
frame_bucket_t stomp_frame_bucket;

static frame_t *alloc_frame() {
  frame_t *ret;

  ret = (frame_t *)malloc(sizeof(frame_t));
  if(ret == NULL) {
    return NULL;
  }

  /* Initialize frame_t object */
  INIT_LIST_HEAD(&ret->l_attrs_h);
  ret->body = NULL;
  ret->status = STATUS_INIT;

  return ret;
}

static void free_frame(frame_t *frame) {
  frame_attr_t *attr;

  list_for_each_entry(attr, &frame->l_attrs_h, l_frame) {
    free(attr);
  }
  list_del(&frame->l_bucket);

  free(frame);
}

int stomp_cleanup() {
  frame_t *frame, *h;

  pthread_mutex_lock(&stomp_frame_bucket.mutex);

  list_for_each_entry_safe(frame, h, &stomp_frame_bucket.l_frame_h, l_bucket) {
    free_frame(frame);
  }

  pthread_mutex_unlock(&stomp_frame_bucket.mutex);

  return RET_SUCCESS;
}

int stomp_init_bucket() {
  INIT_LIST_HEAD(&stomp_frame_bucket.l_frame_h);
  pthread_mutex_init(&stomp_frame_bucket.mutex, NULL);

  return RET_SUCCESS;
}

void *stomp_init_connection(int sock) {
  frame_t *frame = alloc_frame();
  if(frame == NULL) {
    printf("[warning] failed to allocate frame_t\n");
    return RET_ERROR;
  }

  frame->sock = sock;

  pthread_mutex_lock(&stomp_frame_bucket.mutex);
  list_add_tail(&frame->l_bucket, &stomp_frame_bucket.l_frame_h);
  pthread_mutex_unlock(&stomp_frame_bucket.mutex);

  return (void *)frame;
}

int stomp_recv_data(char *recv_data, int len, void *data) {
  char *line, *pointer;
  char *curr = recv_data;
  frame_t *frame = (frame_t *)data;

  for(pointer = recv_data; line = strtok(pointer, "\n"); pointer += strlen(line) + 1) {
    int attrlen = strlen(line);
    frame_attr_t *attr;

    if(attrlen > ATTR_LEN) {
      attrlen = ATTR_LEN;
    }

    attr = (frame_attr_t *)malloc(sizeof(frame_attr_t));
    if(attr == NULL) {
      printf("[warning] failed to allocate frame_attr_t\n");
      break;
    }
    memset(attr->data, 0, ATTR_LEN);
    memcpy(attr->data, recv_data, attrlen);

    list_add_tail(&attr->l_frame, &frame->l_attrs_h);
  }
}
