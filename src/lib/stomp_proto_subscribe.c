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
  int sock;
};

static struct attrinfo_t *alloc_attrinfo() {
  struct attrinfo_t *obj;

  obj = (struct attrinfo_t *)malloc(sizeof(struct attrinfo_t));
  if(obj != NULL) {
    memset(obj, 0, sizeof(struct attrinfo_t));
  }

  return obj;
}

static void free_attrinfo(struct attrinfo_t *obj) {
  if(obj != NULL) {
    free(obj->destination);
    free(obj->id);
    free(obj);
  }
}

static int handler_destination(char *context, void *data, linedata_t *_hdr) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;
  int ret = RET_ERROR;

  if(attrinfo != NULL) {
    attrinfo->destination = (char *)malloc(strlen(context));
    if(attrinfo->destination != NULL) {
      memcpy(attrinfo->destination, context, strlen(context));
    }

    ret = RET_SUCCESS;
  }

  return ret;
}

static int handler_id(char *context, void *data, linedata_t *_hdr) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;
  int ret = RET_ERROR;

  if(attrinfo != NULL) {
    attrinfo->id = (char *)malloc(strlen(context));
    if(attrinfo-> id != NULL) {
      memcpy(attrinfo->id, context, strlen(context));
    }

    ret = RET_SUCCESS;
  }

  return ret;
}

static stomp_header_handler_t handlers[] = {
  {"destination:", handler_destination},
  {"id:", handler_id},
  {0},
};

static void *send_message_worker(void *data) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;
  struct list_head headers;
  linedata_t *body;
  frame_t *frame;
  char *buf;
  int len;

  INIT_LIST_HEAD(&headers);

  buf = (char *)malloc(LD_MAX);
  if(buf != NULL) {

    if(attrinfo->id != NULL) {
      len = sprintf(buf, "subscription: %s\n", attrinfo->id);
      stomp_setdata(buf, len, &headers, NULL);
    }

    len = sprintf(buf, "destination: %s\n", attrinfo->destination);
    stomp_setdata(buf, len, &headers, NULL);

    while(is_socket_valid(attrinfo->sock) == RET_SUCCESS) {

      if((frame = (frame_t *)dequeue(attrinfo->destination)) != NULL) {
        stomp_send_message(attrinfo->sock, frame, &headers);

        free_frame(frame);
      }
    }

    free(buf);
  }

  free_attrinfo(attrinfo);

  return NULL;
}

frame_t *handler_stomp_subscribe(frame_t *frame) {
  struct attrinfo_t *attrinfo;
  pthread_t thread_id;

  assert(frame != NULL);
  assert(frame->cinfo != NULL);

  attrinfo = alloc_attrinfo();
  if(attrinfo == NULL) {
    return NULL;
  }

  /* initialize stomp_message_info_t object */
  attrinfo->sock = frame->sock;

  if(iterate_header(&frame->h_attrs, handlers, attrinfo) == RET_ERROR) {
    err("(handle_stomp_subscribe) validation error");
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  if(attrinfo->destination == NULL) {
    stomp_send_error(frame->sock, "no destination is specified\n");
    return NULL;
  }

  pthread_create(&thread_id, NULL, send_message_worker, attrinfo);

  if(attrinfo->id != NULL) {
    register_subscriber(attrinfo->id, thread_id);
  }

  return NULL;
}
