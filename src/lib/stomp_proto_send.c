#include <newt/list.h>
#include <newt/stomp.h>
#include <newt/common.h>
#include <newt/logger.h>
#include <newt/stomp_sending_worker.h>

#include <assert.h>

#define UNIQUE_STR_LEN 10

struct attrinfo_t {
  stomp_conninfo_t *cinfo;
  char *qname;
  char *tid;
  char *receipt_id;
  char *reply_to;
};

static int handler_destination(char *context, void *data, linedata_t *_hdr) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;

  attrinfo->qname = context;

  return RET_SUCCESS;
}

static int handler_transaction(char *context, void *data, linedata_t *_hdr) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;

  attrinfo->tid = context;

  return RET_SUCCESS;
}

static int handler_receipt(char *context, void *data, linedata_t *_hdr) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;

  attrinfo->receipt_id = context;

  return RET_SUCCESS;
}

static int handler_reply_to(char *context, void *data, linedata_t *hdr) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;
  int context_len = (int)strlen(context);
  int ret = RET_ERROR;

  assert(attrinfo != NULL);

  if(context_len + UNIQUE_STR_LEN < LD_MAX) {
    attrinfo->reply_to = context;

    sprintf(context, "%s%s", context, attrinfo->cinfo->id);
    hdr->len += CONN_ID_LEN;

    ret = RET_SUCCESS;
  }

  return ret;
}

static stomp_header_handler_t handlers[] = {
  {"destination:", handler_destination},
  {"transaction:", handler_transaction},
  {"receipt:", handler_receipt},
  {"reply-to:", handler_reply_to},
  {0},
};

frame_t *handler_stomp_send(frame_t *frame) {
  struct attrinfo_t attrinfo = {0};

  assert(frame != NULL);
  assert(frame->cinfo != NULL);

  attrinfo.cinfo = frame->cinfo;

  if(iterate_header(&frame->h_attrs, handlers, &attrinfo) == RET_ERROR) {
    err("(handle_stomp_send) validation error");
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  if(attrinfo.reply_to != NULL) {
    register_reply_worker(frame->sock, attrinfo.reply_to);
  }

  if(attrinfo.qname == NULL) {
    stomp_send_error(frame->sock, "no destination is specified\n");
    return NULL;
  }

  if(attrinfo.tid == NULL) {
    enqueue((void *)frame, attrinfo.qname);
  } else {
    frame->transaction_data = (void *)attrinfo.qname;
    transaction_add(attrinfo.tid, frame);
  }

  if(attrinfo.receipt_id != NULL) {
    stomp_send_receipt(frame->sock, attrinfo.receipt_id);
  }

  return frame;
}
