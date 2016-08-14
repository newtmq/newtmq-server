#include <newt/stomp.h>
#include <newt/frame.h>
#include <newt/list.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

frame_t *alloc_frame() {
  frame_t *ret;

  ret = (frame_t *)malloc(sizeof(frame_t));
  if(ret == NULL) {
    return NULL;
  }

  /* Initialize frame_t object */
  memset(ret->name, 0, FNAME_LEN);
  memset(ret->id, 0, FRAME_ID_LEN);

  INIT_LIST_HEAD(&ret->h_attrs);
  INIT_LIST_HEAD(&ret->h_data);
  INIT_LIST_HEAD(&ret->l_bucket);
  INIT_LIST_HEAD(&ret->l_transaction);
  INIT_LIST_HEAD(&ret->l_persistent);

  pthread_mutex_init(&ret->mutex_header, NULL);
  pthread_mutex_init(&ret->mutex_body, NULL);

  ret->sock = 0;
  ret->cinfo = NULL;
  ret->status = STATUS_BORN;
  ret->contentlen = -1;
  ret->has_contentlen = 0;
  ret->size = 0;

  ret->transaction_callback = NULL;
  ret->transaction_data = NULL;

  return ret;
}

void free_frame(frame_t *frame) {
  linedata_t *data, *l;

  /* delete header */
  if(! list_empty(&frame->h_attrs)) {
    list_for_each_entry_safe(data, l, &frame->h_attrs, list) {
      list_del(&data->list);
      free(data);
    }
  }

  /* delete body */
  if(! list_empty(&frame->h_data)) {
    list_for_each_entry_safe(data, l, &frame->h_data, list) {
      list_del(&data->list);
      free(data);
    }
  }

  if(frame->transaction_data != NULL) {
    free(frame->transaction_data);
  }

  free(frame);
}
