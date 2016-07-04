#include <newt/list.h>
#include <newt/stomp.h>
#include <newt/common.h>
#include <newt/logger.h>
#include <newt/queue.h>

#include <newt/stomp_management_worker.h>

#include <assert.h>

struct attrinfo_t {
  char *destination;
  char *id;
  char is_topic;
};

static int handler_destination(char *context, void *data, linedata_t *_hdr) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;

  attrinfo->destination = context;

  return RET_SUCCESS;
}

static int handler_id(char *context, void *data, linedata_t *_hdr) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;

  attrinfo->id = context;

  return RET_SUCCESS;
}

static stomp_header_handler_t handlers[] = {
  {"destination:", handler_destination},
  {"id:", handler_id},
  {0},
};

frame_t *handler_stomp_subscribe(frame_t *frame) {
  struct attrinfo_t attrinfo = {0};
  pthread_t thread_id;

  assert(frame != NULL);
  assert(frame->cinfo != NULL);

  if(iterate_header(&frame->h_attrs, handlers, &attrinfo) == RET_ERROR) {
    err("(handle_stomp_subscribe) validation error");
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  if(attrinfo.destination == NULL) {
    stomp_send_error(frame->sock, "no destination is specified\n");
    return NULL;
  }

  stomp_sending_register(frame->sock, attrinfo.destination, attrinfo.id);

  return NULL;
}
