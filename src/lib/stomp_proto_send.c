#include <kazusa/list.h>
#include <kazusa/stomp.h>
#include <kazusa/common.h>
#include <kazusa/logger.h>

#include <assert.h>

#define QNAME_LENGTH (256)

struct dstinfo_t {
  char *qname;
};

static int handler_destination(char *context, void *data) {
  struct dstinfo_t *dstinfo = (struct dstinfo_t *)data;
  int ret = RET_ERROR;

  if(dstinfo != NULL) {
    dstinfo->qname = context;

    ret = RET_SUCCESS;
  }

  return ret;
}

static stomp_header_handler_t handlers[] = {
  {"destination:", handler_destination},
  {0},
};

frame_t *handler_stomp_send(frame_t *frame) {
  struct dstinfo_t dstinfo = {0};

  assert(frame != NULL);
  assert(frame->cinfo != NULL);

  if(iterate_header(&frame->h_attrs, handlers, &dstinfo) == RET_ERROR) {
    err("(handle_stomp_send) validation error");
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  if(dstinfo.qname == NULL) {
    stomp_send_error(frame->sock, "no destination is specified\n");
    return NULL;
  }

  linedata_t *ldata = list_first_entry(&frame->h_data, linedata_t, l_frame);
  debug("(handle_stomp_send) enqueued : %s", ldata->data);

  enqueue((void *)frame, dstinfo.qname);

  return frame;
}
