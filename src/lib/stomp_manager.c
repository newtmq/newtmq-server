#include <kazusa/stomp.h>
#include <kazusa/common.h>

frame_t *get_frame_from_bucket() {
  frame_t *frame = NULL;

  pthread_mutex_lock(&stomp_frame_bucket.mutex);
  { /* thread safe */
    if(! list_empty(&stomp_frame_bucket)) {
      frame = list_entry(stomp_frame_bucket.next, frame_t, l_bucket);
      list_del(&frame->l_bucket);
    }
  }
  pthread_mutex_unlock(&stomp_frame_bucket.mutex);

  return frame;
}

void *stomp_manager(void *data) {
  frame_t *frame;
  while(1) {
    frame = get_frame_from_bucket();
    if(frame != NULL) {
      // handle CONNECT or STOMP
      // hanale SEND
      // hanale SUBSCRIBE
      // hanale UNSUBSCRIBE
      // hanale BEGIN
      // hanale COMMIT
      // hanale ABORT
      // hanale ACK
      // hanale NACK
      // hanale DISCONNECT
    }
  }

  return NULL;
}
