#include <newt/common.h>
#include <newt/transaction.h>
#include <newt/frame.h>

#include <assert.h>

static int handler_transaction(char *context, void *data, linedata_t *_hdr) {
  char **tid = (char **)data;
  int ret = RET_ERROR;

  if(context != NULL && strlen(context) > 0) {
    *tid = context;

    ret = RET_SUCCESS;
  }

  return ret;
}

static stomp_header_handler_t handlers[] = {
  {"transaction:", handler_transaction},
  {0},
};

frame_t *handler_stomp_begin(frame_t *frame) {
  char *transaction_id = NULL;

  assert(frame != NULL);

  if(iterate_header(&frame->h_attrs, handlers, &transaction_id) == RET_ERROR) {
    err("(handle_stomp_ack) validation error");
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  if(transaction_id != NULL) {
    transaction_start(transaction_id);
  } else {
    stomp_send_error(frame->sock, "BEGIN frame MUST be specified transaction header");
  }

  return NULL;
}
