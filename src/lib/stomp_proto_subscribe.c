#include <kazusa/list.h>
#include <kazusa/stomp.h>
#include <kazusa/common.h>
#include <kazusa/logger.h>

#include <kazusa/stomp_message_worker.h>

#include <assert.h>

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

frame_t *handler_stomp_subscribe(frame_t *frame) {
  struct dstinfo_t dstinfo = {0};
  stomp_msginfo_t *msginfo;

  assert(frame != NULL);
  assert(frame->cinfo != NULL);

  if(iterate_header(&frame->h_attrs, handlers, &dstinfo) == RET_ERROR) {
    err("(handle_stomp_subscribe) validation error");
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  if(dstinfo.qname == NULL) {
    stomp_send_error(frame->sock, "no destination is specified\n");
    return NULL;
  }

  msginfo = alloc_msginfo();
  if(msginfo != NULL) {
    int qname_len = strlen(dstinfo.qname);

    if(qname_len > LD_MAX) {
      qname_len = LD_MAX;
    }

    msginfo->sock = frame->sock;
    strncpy(msginfo->qname, dstinfo.qname, qname_len);

    debug("(handle_stomp_subscribe) message_register");

    stomp_message_register(msginfo);
  }

  return NULL;
}
