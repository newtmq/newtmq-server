#include <newt/common.h>
#include <newt/stomp.h>

#include <assert.h>

static int handler_receipt(char *context, void *data, linedata_t *_hdr) {
  char **receipt_id = (char **)data;
  int ret = RET_ERROR;

  if(context != NULL && strlen(context) > 0) {
    *receipt_id = context;

    ret = RET_SUCCESS;
  }

  return ret;
}

static stomp_header_handler_t handlers[] = {
  {"receipt:", handler_receipt},
  {0},
};

frame_t *handler_stomp_disconnect(frame_t *frame) {
  char *receipt_id = NULL;

  assert(frame != NULL);

  if(iterate_header(&frame->h_attrs, handlers, &receipt_id) == RET_ERROR) {
    err("(handle_stomp_ack) validation error");
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  if(receipt_id != NULL) {
    stomp_send_receipt(frame->sock, receipt_id);
  }

  return NULL;
}
