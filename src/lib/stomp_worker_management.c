#include <kazusa/common.h>
#include <kazusa/logger.h>
#include <kazusa/stomp.h>
#include <kazusa/stomp_management_worker.h>

typedef struct stomp_handler_t {
  char *name;
  frame_t *(*handler)(frame_t *);
} stomp_handler_t;

static stomp_handler_t stomp_handlers[] = {
  {"SEND",        handler_stomp_send},
  {"SUBSCRIBE",   handler_stomp_subscribe},
  {"CONNECT",     handler_stomp_connect},
  {"STOMP",       handler_stomp_connect},
  {"DISCONNECT",  NULL}, // not implemented yet
  {"UNSUBSCRIBE", NULL}, // not implemented yet
  {"BEGIN",       NULL}, // not implemented yet
  {"COMMIT",      NULL}, // not implemented yet
  {"ABORT",       NULL}, // not implemented yet
  {"ACK",         NULL}, // not implemented yet
  {"NACK",        NULL}, // not implemented yet
  {0},
};

static int handle_frame(frame_t *frame) {
  int i;
  stomp_handler_t *h;

  for(i=0; h=&stomp_handlers[i], h->name!=NULL; i++) {
    if(strncmp(frame->name, h->name, strlen(h->name)) == 0 && h->handler != NULL) {
      /* The incomed frame is processed by the STOMP frame handlers which is correspoing to it */
      frame_t *ret = (*h->handler)(frame);
      if(ret == NULL) {
        free_frame(frame);
      }

      break;
    }
  }
}

int iterate_header(struct list_head *h_header, stomp_header_handler_t *handlers, void *data) {
  linedata_t *line;

  list_for_each_entry(line, h_header, l_frame) {
    stomp_header_handler_t *h;
    int i;

    for(i=0; h=&handlers[i], h->name!=NULL; i++) {
      if(strncmp(line->data, h->name, strlen(h->name)) == 0) {
        int ret = (*h->handler)((line->data + strlen(h->name)), data);
        if(ret == RET_ERROR) {
          return RET_ERROR;
        }
      }
    }
  }

  return RET_SUCCESS;
}

void *stomp_management_worker(void *data) {
  frame_t *frame;

  while(1) {
    frame = get_frame_from_bucket();
    if(frame != NULL) {
      logger(LOG_DEBUG, "(stomp_management_worker) frame_name: %s", frame->name);

      handle_frame(frame);
    }
  }

  return NULL;
}
