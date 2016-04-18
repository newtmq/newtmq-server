#include <kazusa/list.h>
#include <kazusa/stomp.h>
#include <kazusa/common.h>

#include <assert.h>

#define QNAME_LENGTH (256)

struct sendinfo_t {
  char *qname;
};

static int handler_destination(char *context, void *data) {
  struct sendinfo_t *sendinfo = (struct sendinfo_t *)data;
  int ret = RET_ERROR;

  if(sendinfo != NULL) {
    sendinfo->qname = context;

    ret = RET_SUCCESS;
  }

  return ret;
}

static stomp_header_handler_t handlers[] = {
  {"destination:", handler_destination},
  {0},
};

frame_t *handler_stomp_send(frame_t *frame) {
  struct sendinfo_t sendinfo = {0};

  assert(frame != NULL);
  assert(frame->cinfo != NULL);

  if(iterate_header(&frame->h_attrs, handlers, &sendinfo) == RET_ERROR) {
    printf("[ERROR] (handle_stomp_send) validation error\n");
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  if(sendinfo.qname == NULL) {
    stomp_send_error(frame->sock, "no destination is specified\n");
    return NULL;
  }

  printf("[debug] (handle_stomp_send) store data to queue ('%s')\n", sendinfo.qname);

  enqueue((void *)&frame->h_data, sendinfo.qname);

  return NULL;
}
