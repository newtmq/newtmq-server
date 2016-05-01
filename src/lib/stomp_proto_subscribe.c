#include <kazusa/list.h>
#include <kazusa/stomp.h>
#include <kazusa/common.h>
#include <kazusa/logger.h>
#include <kazusa/queue.h>

#include <kazusa/stomp_management_worker.h>

#include <assert.h>

static int handler_destination(char *context, void *data) {
  stomp_msginfo_t *msginfo = (stomp_msginfo_t *)data;
  int ret = RET_ERROR;

  if(msginfo != NULL) {
    memcpy(msginfo->qname, context, strlen(context));
    SET(msginfo, DESTINATION_IS_SET);

    ret = RET_SUCCESS;
  }

  return ret;
}

static int handler_id(char *context, void *data) {
  stomp_msginfo_t *msginfo = (stomp_msginfo_t *)data;
  int ret = RET_ERROR;

  if(msginfo != NULL) {
    memcpy(msginfo->id, context, strlen(context));
    SET(msginfo, ID_IS_SET);

    ret = RET_SUCCESS;
  }

  return ret;
}

static stomp_header_handler_t handlers[] = {
  {"destination:", handler_destination},
  {"id:", handler_id},
  {0},
};

void *send_message_worker(void *data) {
  stomp_msginfo_t *msginfo = (stomp_msginfo_t *)data;
  frame_t *frame;
  int index = 0;
  char hdr_dest[LD_MAX];
  char hdr_msgid[LD_MAX];
  linedata_t *body;

  assert(msginfo != NULL);

  debug("(send_message_worker) msginfo->sock: %d", msginfo->sock);

  while(is_socket_valid(msginfo->sock) == RET_SUCCESS) {
    if((frame = (frame_t *)dequeue(msginfo->qname)) != NULL) {
      /* making message-id attribute for each message */
      sprintf(hdr_msgid, "message-id: %s-%d\n", msginfo->qname, index++);
      debug("(send_message_worker) msg-id: %s", hdr_msgid);

      send_msg(msginfo->sock, "MESSAGE\n");
      send_msg(msginfo->sock, hdr_dest);
      send_msg(msginfo->sock, hdr_msgid);
      send_msg(msginfo->sock, "\n");
      list_for_each_entry(body, &frame->h_data, l_frame) {
        send_msg(msginfo->sock, body->data);
        send_msg(msginfo->sock, "\n");
      }
      send_msg(msginfo->sock, NULL);

      free_frame(frame);
    }
  }

  free_msginfo(msginfo);

  return NULL;
}

frame_t *handler_stomp_subscribe(frame_t *frame) {
  stomp_msginfo_t *msginfo;
  pthread_t thread_id;

  assert(frame != NULL);
  assert(frame->cinfo != NULL);

  msginfo = alloc_msginfo();
  if(msginfo == NULL) {
    return NULL;
  }

  /* initialize stomp_message_info_t object */
  msginfo->sock = frame->sock;

  if(iterate_header(&frame->h_attrs, handlers, msginfo) == RET_ERROR) {
    err("(handle_stomp_subscribe) validation error");
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  if(! GET(msginfo, DESTINATION_IS_SET)) {
    stomp_send_error(frame->sock, "no destination is specified\n");
    return NULL;
  }

  debug("(handle_stomp_subscribe) sock: %d [%d]", frame->sock, msginfo->sock);

  pthread_create(&thread_id, NULL, send_message_worker, msginfo);

  if(GET(msginfo, ID_IS_SET)) {
    register_subscriber(msginfo->id, thread_id);
  }

  return NULL;
}
