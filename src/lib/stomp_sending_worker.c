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

struct worker_manager_t {
  pthread_mutex_t mutex;
  struct list_head head;
};

// This data structure exsists per destination for adjusting each clients to get frames from correspoing queue
struct dest_group_t {
  char *destination;
  int len;
  struct list_head list;
  struct list_head h_client;
  pthread_mutex_t m_client;
  int status;
};

// This describes client interface information
struct client_info_t {
  int sock;
  // This parameter uses to quit sending frames to target socket by UNSUBSCRIBE frame, or something.
  char *id;
  struct dest_group_t *dgroup;
  struct list_head list;
};

static struct worker_manager_t worker_manager = {};

int initialize_sending_worker() {
  int ret = RET_SUCCESS;

  INIT_LIST_HEAD(&worker_manager.head);
  pthread_mutex_init(&worker_manager.mutex, NULL);

  return ret;
}

// this function checks same target thread is already created, or not.
// returns target pointer of dest_group_t object or NULL
static struct dest_group_t *search_dest_group(char *dest, int len) {
  struct dest_group_t *dgroup, *ret = NULL;

  pthread_mutex_lock(&worker_manager.mutex);
  {
    list_for_each_entry(dgroup, &worker_manager.head, list) {
      if(len == dgroup->len && strncmp(dgroup->destination, dest, len) == 0) {
        ret = dgroup;
        break;
      }
    }
  }
  pthread_mutex_unlock(&worker_manager.mutex);

  return ret;
}

static struct dest_group_t *alloc_dest_group() {
  struct dest_group_t *dgroup;

  dgroup = (struct dest_group_t *)malloc(sizeof(struct dest_group_t));
  if(dgroup != NULL) {
    memset(dgroup, 0, sizeof(struct dest_group_t));
    pthread_mutex_init(&dgroup->m_client, NULL);
    INIT_LIST_HEAD(&dgroup->list);
    INIT_LIST_HEAD(&dgroup->h_client);
    CLR(dgroup);
  }

  return dgroup;
}

static int initialize_dest_group(struct dest_group_t *dgroup, char *destination, int len) {
  assert(destination != NULL);
  assert(dgroup != NULL);

  dgroup->destination = (char *)malloc(len + 1);
  memcpy(dgroup->destination, destination, len);
  dgroup->destination[len] = '\0';
  dgroup->len = len;

  pthread_mutex_lock(&worker_manager.mutex);
  {
    list_add_tail(&dgroup->list, &worker_manager.head);
  }
  pthread_mutex_unlock(&worker_manager.mutex);

  return RET_SUCCESS;
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

static void free_client_info(struct client_info_t *cinfo) {
  free(cinfo->id);
  free(cinfo);
}

static void free_dest_group(struct dest_group_t *dgroup) {
  if(dgroup != NULL) {
    free(dgroup->destination);

    struct client_info_t *cinfo, *c;
    list_for_each_entry_safe(cinfo, c, &dgroup->h_client, list) {
      if(cinfo->id) {
        free(cinfo->id);
      }
      list_del(&cinfo->list);
      free_client_info(cinfo);
    }
    free(dgroup);
  }
}

static void *reply_worker(void *arg) {
  struct client_info_t *cinfo = (struct client_info_t *)arg;
  struct dest_group_t *dgroup;
  struct list_head headers;

  assert(cinfo != NULL);

  dgroup = cinfo->dgroup;
  while(is_socket_valid(cinfo->sock) == RET_SUCCESS) {
    frame_t *frame;

    if((frame = (frame_t *)dequeue(dgroup->destination)) != NULL) {
      stomp_send_message(cinfo->sock, frame, NULL);
    }
  }

  return NULL;
}

static void *unicast_worker(void *arg) {
  struct client_info_t *cinfo = (struct client_info_t *)arg;
  struct dest_group_t *dgroup;
  struct list_head *headers = NULL;
  char buf[LD_MAX];

  assert(cinfo != NULL);

  // This processing is needed when id parameter is specified
  if(cinfo->id != NULL) {
    headers = (struct list_head *)malloc(sizeof(struct list_head));

    INIT_LIST_HEAD(headers);

    memset(buf, 0, LD_MAX);
    int len = sprintf(buf, "subscription: %s", cinfo->id);
    stomp_setdata(buf, len, headers, NULL);
  }

  dgroup = cinfo->dgroup;
  while(is_socket_valid(cinfo->sock) == RET_SUCCESS) {
    frame_t *frame;

    pthread_mutex_lock(&dgroup->m_client);
    {
      frame = (frame_t *)dequeue(dgroup->destination);
    }
    pthread_mutex_unlock(&dgroup->m_client);

    if(frame != NULL) {
      stomp_send_message(cinfo->sock, frame, headers);
    }
  }

  return NULL;
}

static void *multicast_worker(void *arg) {
  struct dest_group_t *dgroup = (struct dest_group_t *)arg;
  struct client_info_t *cinfo;
  struct list_head headers;
  char buf[LD_MAX];

  assert(dgroup != NULL);

  while(! list_empty(&dgroup->h_client)) {
    frame_t *frame;
    if((frame = (frame_t *)dequeue(dgroup->destination)) != NULL) {
      struct client_info_t *cinfo, *c;
      list_for_each_entry_safe(cinfo, c, &dgroup->h_client, list) {
        if(is_socket_valid(cinfo->sock) == RET_SUCCESS) {
          INIT_LIST_HEAD(&headers);

          // This processing is needed when id parameter is specified
          if(cinfo->id != NULL) {
            memset(buf, 0, LD_MAX);
            int len = sprintf(buf, "subscription: %s", cinfo->id);
            stomp_setdata(buf, len, &headers, NULL);
          }
          stomp_send_message(cinfo->sock, frame, &headers);
        } else {
          pthread_mutex_lock(&dgroup->m_client);
          {
            list_del(&cinfo->list);
          }
          pthread_mutex_unlock(&dgroup->m_client);
        }
      }
      free_frame(frame);
    }
  }

  return NULL;
}

// This is a worker that dedicate to send MESSAGE frame in the specified queue to the specific client.
int register_reply_worker(int sock, char *destination) {
  struct dest_group_t *dgroup;
  struct client_info_t *cinfo;
  int len;
  pthread_t thread_id;

  assert(destination != NULL);

  len = (int)strlen(destination);

  // get or create dest_group_t object
  dgroup = search_dest_group(destination, len);
  if(dgroup == NULL) {
    dgroup = alloc_dest_group(destination, len);

    initialize_dest_group(dgroup, destination, len);

    // register client info
    cinfo = alloc_client_info();
    cinfo->sock = sock;
    cinfo->dgroup = dgroup;

    pthread_mutex_lock(&dgroup->m_client);
    {
      list_add_tail(&cinfo->list, &dgroup->h_client);
    }
    pthread_mutex_unlock(&dgroup->m_client);

    pthread_create(&thread_id, NULL, reply_worker, cinfo);
  }

  return RET_SUCCESS;
}

int register_unicast_worker(int sock, char *destination, char *id) {
  struct dest_group_t *dgroup;
  struct client_info_t *cinfo;
  int len;
  pthread_t thread_id;

  assert(destination != NULL);

  len = (int)strlen(destination);
  dgroup = search_dest_group(destination, len);
  if(dgroup == NULL) {
    dgroup = alloc_dest_group(destination, len);

    initialize_dest_group(dgroup, destination, len);
  }

  // register client info
  cinfo = alloc_client_info();
  cinfo->sock = sock;
  cinfo->dgroup = dgroup;

  pthread_mutex_lock(&dgroup->m_client);
  {
    list_add_tail(&cinfo->list, &dgroup->h_client);
  }
  pthread_mutex_unlock(&dgroup->m_client);

  pthread_create(&thread_id, NULL, unicast_worker, cinfo);

  return RET_SUCCESS;
}

int register_multicast_worker(int sock, char *destination, char *id) {
  struct dest_group_t *dgroup;
  struct client_info_t *cinfo;
  int len;
  pthread_t thread_id;

  assert(destination != NULL);

  len = (int)strlen(destination);

  // get or create dest_group_t object
  dgroup = search_dest_group(destination, len);
  if(dgroup == NULL) {
    dgroup = alloc_dest_group(destination, len);

    initialize_dest_group(dgroup, destination, len);

    pthread_create(&thread_id, NULL, multicast_worker, dgroup);
  }

  // register client info
  cinfo = alloc_client_info();
  cinfo->sock = sock;
  cinfo->dgroup = dgroup;

  pthread_mutex_lock(&dgroup->m_client);
  {
    list_add_tail(&cinfo->list, &dgroup->h_client);
  }
  pthread_mutex_unlock(&dgroup->m_client);

  return RET_SUCCESS;
}
