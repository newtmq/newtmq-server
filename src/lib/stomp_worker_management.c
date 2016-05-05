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
  {"DISCONNECT",  NULL}, // not implemented yet
  {"UNSUBSCRIBE", handler_stomp_unsubscribe},
  {"BEGIN",       handler_stomp_begin},
  {"COMMIT",      NULL}, // not implemented yet
  {"ABORT",       NULL}, // not implemented yet
  {"ACK",         handler_stomp_ack},
  {"NACK",        handler_stomp_nack},
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

  initialize_manager();

  while(1) {
    frame = get_frame_from_bucket();
    if(frame != NULL) {
      debug("(stomp_management_worker) frame_name: %s", frame->name);

      handle_frame(frame);
    }
  }

  return NULL;
}

stomp_msginfo_t *alloc_msginfo() {
  stomp_msginfo_t *ret;

  ret = (stomp_msginfo_t *)malloc(sizeof(stomp_msginfo_t));
  if(ret == NULL) {
    err("[alloc_msginfo] failed to allocate memory");
    return NULL;
  }

  /* Initialize object */
  memset(ret, 0, sizeof(stomp_msginfo_t));
  INIT_LIST_HEAD(&ret->list);

  return ret;
}

void free_msginfo(stomp_msginfo_t *msginfo) {
  if(msginfo != NULL) {
    free(msginfo);
  }
}
