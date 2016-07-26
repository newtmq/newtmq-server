#include <newt/common.h>
#include <newt/logger.h>
#include <newt/stomp.h>
#include <newt/stomp_management_worker.h>

typedef struct stomp_handler_t {
  char *name;
  frame_t *(*handler)(frame_t *);
} stomp_handler_t;

static management_t management_info;

static stomp_handler_t stomp_handlers[] = {
  {"SEND",        handler_stomp_send},
  {"SUBSCRIBE",   handler_stomp_subscribe},
  {"CONNECT",     handler_stomp_connect},
  {"STOMP",       handler_stomp_connect},
  {"DISCONNECT",  handler_stomp_disconnect},
  {"UNSUBSCRIBE", handler_stomp_unsubscribe},
  {"BEGIN",       handler_stomp_begin},
  {"COMMIT",      handler_stomp_commit},
  {"ABORT",       handler_stomp_abort},
  {"ACK",         handler_stomp_ack},
  {"NACK",        handler_stomp_nack},
  {0},
};

static void send_handle_error(frame_t *frame) {
  char msg[LD_MAX] = {0};

  if(frame != NULL) {
    sprintf(msg, "failed to handle frame (%s)", frame->name);
    stomp_send_error(frame->sock, msg);
  }
}

static int handle_frame(frame_t *frame) {
  int i;
  /* This value specifies whether target frame is handled or not */
  int is_frame_handled = 0;
  stomp_handler_t *h;

  for(i=0; h=&stomp_handlers[i], h->name!=NULL; i++) {
    if(strncmp(frame->name, h->name, strlen(h->name)) == 0 && h->handler != NULL) {
      /* The incomed frame is processed by the STOMP frame handlers which is correspoing to it */
      frame_t *ret = (*h->handler)(frame);
      if(ret == NULL) {
        free_frame(frame);
      }

      /* indicate that the target frame is handled by STOMP protocol handlers */
      is_frame_handled = 1;

      break;
    }
  }

  if(is_frame_handled == 0) {
    send_handle_error(frame);
  }
}

int initialize_manager() {
  pthread_mutex_init(&management_info.mutex, NULL);
  INIT_LIST_HEAD(&management_info.h_subscribe);

  return RET_SUCCESS;
}

int register_subscriber(char *id, pthread_t tid) {
  subscribe_t *subscribe_info;

  if(id != NULL) {
    int len = LD_MAX;

    subscribe_info = (subscribe_t *)malloc(sizeof(subscribe_t));
    if(subscribe_info == NULL) {
      return RET_ERROR;
    }

    if(strlen(id) < LD_MAX) {
      len = strlen(id);
    }

    INIT_LIST_HEAD(&subscribe_info->list);
    memcpy(subscribe_info->id, id, len);
    subscribe_info->thread_id = tid;

    pthread_mutex_lock(&management_info.mutex);
    {
      list_add(&subscribe_info->list, &management_info.h_subscribe);
    }
    pthread_mutex_unlock(&management_info.mutex);
  }

  return RET_SUCCESS;
}

int unregister_subscriber(char *id) {
  subscribe_t *info = NULL;
  int ret = RET_ERROR;

  if(id != NULL) {
    pthread_mutex_lock(&management_info.mutex);
    {
      if(! list_empty(&management_info.h_subscribe)) {
        info = list_first_entry(&management_info.h_subscribe, subscribe_t, list);
        list_del(&info->list);

        free(info);

        ret = RET_SUCCESS;
      }
    }
    pthread_mutex_unlock(&management_info.mutex);
  }

  return ret;
}

subscribe_t *get_subscriber(char *id) {
  if(id != NULL) {
    if(! list_empty(&management_info.h_subscribe)) {
      return list_first_entry(&management_info.h_subscribe, subscribe_t, list);
    }
  }
  return NULL;
}

int iterate_header(struct list_head *h_header, stomp_header_handler_t *handlers, void *data) {
  linedata_t *line;

  list_for_each_entry(line, h_header, list) {
    stomp_header_handler_t *h;
    int i;

    for(i=0; h=&handlers[i], h->name!=NULL; i++) {
      if(strncmp(line->data, h->name, strlen(h->name)) == 0) {
        int ret = (*h->handler)((line->data + strlen(h->name)), data, line);
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

  initialize_manager();

  while(1) {
    frame = get_frame_from_bucket();
    if(frame != NULL) {
      debug("(stomp_management_worker) (%p) frame_name: %s", frame, frame->name);

      handle_frame(frame);
    }
  }

  return NULL;
}
