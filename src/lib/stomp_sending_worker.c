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
#include <newt/stomp_sending_worker.h>

// this value is used at status parmaeter of worker_info object
enum worker_status {
  IS_TOPIC = 1 << 0,
};

struct worker_manager_t {
  pthread_mutex_t mutex;
  struct list_head head;
};

// This describes client interface information
struct client_info_t {
  int sock;
  // This parameter uses to quit sending frames to target socket by UNSUBSCRIBE frame, or something.
  char *id;
  struct list_head list;
};

// This data structure has informations about worker thread (allocated per worker threads).
struct worker_info_t {
  char *destination;
  int len;
  struct list_head list;
  struct list_head h_client;
  pthread_t thread_id;
  pthread_mutex_t m_client;
  int status;
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
static struct worker_info_t *get_worker_info(char *dest, int len) {
  struct worker_info_t *winfo, *ret = NULL;

  pthread_mutex_lock(&worker_manager.mutex);
  {
    list_for_each_entry(winfo, &worker_manager.head, list) {
      if(len == winfo->len && strncmp(winfo->destination, dest, len) == 0) {
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
    pthread_mutex_init(&winfo->m_client, NULL);
    INIT_LIST_HEAD(&winfo->list);
    INIT_LIST_HEAD(&winfo->h_client);
    CLR(winfo);
  }

  return winfo;
}

static struct client_info_t *alloc_client_info() {
  struct client_info_t *cinfo;

  cinfo = (struct client_info_t *)malloc(sizeof(struct client_info_t));
  if(cinfo != NULL) {
    INIT_LIST_HEAD(&cinfo->list);
    cinfo->id = NULL;
    cinfo->sock = 0;
  }

  return cinfo;
}

static void free_worker_info(struct worker_info_t *winfo) {
  if(winfo != NULL) {
    if(winfo->destination != NULL) {
      free(winfo->destination);
    }

    struct client_info_t *cinfo, *c;
    list_for_each_entry_safe(cinfo, c, &winfo->h_client, list) {
      if(cinfo->id) {
        free(cinfo->id);
      }
      list_del(&cinfo->list);
      free(cinfo);
    }
    free(winfo);
  }
}

static int multicast_message(struct worker_info_t *winfo) {
  struct list_head headers;
  char buf[LD_MAX];
  frame_t *frame;

  INIT_LIST_HEAD(&headers);

  while(! list_empty(&winfo->h_client)) {
    if((frame = (frame_t *)dequeue(winfo->destination)) != NULL) {
      struct client_info_t *cinfo, *c;
      list_for_each_entry_safe(cinfo, c, &winfo->h_client, list) {
        if(is_socket_valid(cinfo->sock) == RET_SUCCESS) {
          if(cinfo->id != NULL) {
            memset(buf, 0, LD_MAX);
            int len = sprintf(buf, "subscription: %s", cinfo->id);
            stomp_setdata(buf, len, &headers, NULL);
          }

          stomp_send_message(cinfo->sock, frame, &headers);
        } else {
          pthread_mutex_lock(&winfo->m_client);
          {
            list_del(&cinfo->list);
          }
          pthread_mutex_unlock(&winfo->m_client);
        }
      }
    }
  }

  return RET_SUCCESS;
}

static int unicast_message(struct worker_info_t *winfo) {
  struct client_info_t *cinfo;
  struct list_head headers;
  frame_t *frame;
  char buf[LD_MAX];

  INIT_LIST_HEAD(&headers);

  struct list_head *curr = winfo->h_client.next;
  while(! list_empty(&winfo->h_client)) {
    cinfo = list_entry(curr, struct client_info_t, list);
    // set next pointer
    pthread_mutex_lock(&winfo->m_client);
    {
      curr = curr->next;
      if(curr == &winfo->h_client) {
        curr = winfo->h_client.next;
      }
    }
    pthread_mutex_unlock(&winfo->m_client);

    if(cinfo->id != NULL) {
      memset(buf, 0, LD_MAX);
      int len = sprintf(buf, "subscription: %s", cinfo->id);
      stomp_setdata(buf, len, &headers, NULL);
    }

    if(is_socket_valid(cinfo->sock) != RET_SUCCESS) {
      pthread_mutex_lock(&winfo->m_client);
      {
        list_del(&cinfo->list);
      }
      pthread_mutex_unlock(&winfo->m_client);
    } else {
      if((frame = (frame_t *)dequeue(winfo->destination)) != NULL) {
        stomp_send_message(cinfo->sock, frame, &headers);

        free_frame(frame);
      }
    }
  }

  return RET_SUCCESS;
}

static void *worker_sending(void *data) {
  struct worker_info_t *winfo = (struct worker_info_t *)data;

  assert(winfo != NULL);
  assert(winfo->destination != NULL);

  // Because inter main-loop is too hot, so it should be keeped simple.
  if(GET(winfo, IS_TOPIC)) {
    multicast_message(winfo);
  } else {
    unicast_message(winfo);
  }

  pthread_mutex_lock(&worker_manager.mutex);
  {
    list_del(&winfo->list);
  }
  pthread_mutex_unlock(&worker_manager.mutex);

  free_worker_info(winfo);

  return NULL;
}

// This is a worker that dedicate to send MESSAGE frame in the specified queue to the specific client.
int stomp_sending_register(int sock, char *destination, char *id) {
  struct worker_info_t *winfo;
  struct client_info_t *cinfo;

  assert(destination != NULL);

  int len = (int)strlen(destination);
  winfo = get_worker_info(destination, len);
  if(winfo == NULL)  {
    debug("[stomp_sending_register] (created) %s [%d]", destination, len);

    winfo = alloc_worker_info();

    // checks destination is topic, or not
    if(strncmp(destination, "/topic/", 7) == 0) {
      SET(winfo, IS_TOPIC);
    }

    winfo->destination = (char *)malloc(len + 1);
    memcpy(winfo->destination, destination, len);
    winfo->destination[len] = '\0';
    winfo->len = len;

    pthread_mutex_lock(&worker_manager.mutex);
    {
      list_add_tail(&winfo->list, &worker_manager.head);
    }
    pthread_mutex_unlock(&worker_manager.mutex);

    pthread_create(&winfo->thread_id, NULL, worker_sending, winfo);
  }

  // register client info
  cinfo = alloc_client_info();
  cinfo->sock = sock;
  if(id != NULL) {
    len = (int)strlen(id);
    cinfo->id = (char *)malloc(len + 1);
    memcpy(cinfo->id, id, len);
    cinfo->id[len] = '\0';
  }
  pthread_mutex_lock(&winfo->m_client);
  {
    list_add_tail(&cinfo->list, &winfo->h_client);
  }
  pthread_mutex_unlock(&winfo->m_client);

  return RET_SUCCESS;
}
