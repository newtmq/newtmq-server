#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <newt/list.h>
#include <newt/stomp.h>
#include <newt/queue.h>
#include <newt/logger.h>
#include <newt/common.h>
#include <newt/stomp_worker_sending.h>

struct worker_manager_t {
  pthread_mutex_t mutex;
  struct list_head head;
};

struct worker_info_t {
  int sock;
  char *destination;
  char *id;
  struct list_head list;
};

static struct worker_manager_t worker_manager = {};

int initialize_worker_sending() {
  int ret = RET_SUCCESS;

  INIT_LIST_HEAD(&worker_manager.head);
  pthread_mutex_init(&worker_manager.mutex, NULL);

  return ret;
}

// this function checks same target thread is already created, or not.
// returns target pointer of worker_info_t object or NULL
static struct worker_info_t *is_thread_created(char *dest, int len) {
  struct worker_info_t *winfo, *ret = NULL;

  pthread_mutex_lock(&worker_manager.mutex);
  {
    list_for_each_entry(winfo, &worker_manager.head, list) {
      debug("[is_thread_created] (dest) %s", winfo->destination);
      if(strncmp(winfo->destination, dest, len) == 0) {
        ret = winfo;
        break;
      }
    }
  }
  pthread_mutex_unlock(&worker_manager.mutex);

  return ret;
}

static struct worker_info_t *alloc_worker_info() {
  struct worker_info_t *winfo;

  winfo = (struct worker_info_t *)malloc(sizeof(struct worker_info_t));
  if(winfo != NULL) {
    memset(winfo, 0, sizeof(struct worker_info_t));
    INIT_LIST_HEAD(&winfo->list);
  }

  return winfo;
}

static void free_worker_info(struct worker_info_t *info) {
  if(info != NULL) {
    if(info->destination != NULL) {
      free(info->destination);
    }
    if(info->id != NULL) {
      free(info->id);
    }
    free(info);
  }
}

static void *worker_sending(void *data) {
  struct worker_info_t *worker_info = (struct worker_info_t *)data;
  struct list_head headers;
  linedata_t *body;
  frame_t *frame;
  char *buf;
  int len;

  assert(worker_info != NULL);
  assert(worker_info->sock > 0);
  assert(worker_info->destination != NULL);

  INIT_LIST_HEAD(&headers);

  buf = (char *)malloc(LD_MAX);
  if(buf != NULL) {

    if(worker_info->id != NULL) {
      len = sprintf(buf, "subscription: %s", worker_info->id);
      stomp_setdata(buf, len, &headers, NULL);
    }

    /*
    len = sprintf(buf, "destination: %s", worker_info->destination);
    stomp_setdata(buf, len, &headers, NULL);
    */
    
    while(is_socket_valid(worker_info->sock) == RET_SUCCESS) {
      if((frame = (frame_t *)dequeue(worker_info->destination)) != NULL) {
        stomp_send_message(worker_info->sock, frame, &headers);

        free_frame(frame);
      }
    }

    free(buf);
  }

  pthread_mutex_lock(&worker_manager.mutex);
  {
    list_del(&worker_info->list);
  }
  pthread_mutex_unlock(&worker_manager.mutex);

  free_worker_info(worker_info);

  return NULL;
}

// This is a worker that dedicate to send MESSAGE frame in the specified queue to the specific client.
int stomp_sending_register(int sock, char *destination, char *id) {
  struct worker_info_t *worker_info;
  pthread_t thread_id;
  int len;

  assert(destination != NULL);

  debug("[stomp_sending_register] started");

  len = (int)strlen(destination);
  if(is_thread_created(destination, len) == NULL)  {
    debug("[stomp_sending_register] (created) %s [%d]", destination, len);

    worker_info = alloc_worker_info();

    worker_info->sock = sock;
    worker_info->destination = (char *)malloc(len + 1);
    memcpy(worker_info->destination, destination, len);
    worker_info->destination[len] = '\0';

    pthread_mutex_lock(&worker_manager.mutex);
    {
      list_add_tail(&worker_info->list, &worker_manager.head);
    }
    pthread_mutex_unlock(&worker_manager.mutex);

    if(id != NULL) {
      len = (int)strlen(id);
      worker_info->id = (char *)malloc(len + 1);
      memcpy(worker_info->id, id, len);
      worker_info->id[len] = '\0';
    }

    pthread_create(&thread_id, NULL, worker_sending, worker_info);

    if(worker_info->id != NULL) {
      register_subscriber(worker_info->id, thread_id);
    }
  }

  debug("[stomp_sending_register] finished");

  return RET_SUCCESS;
}
