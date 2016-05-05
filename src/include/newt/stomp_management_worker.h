#ifndef __STOMP_MANAGEMENT_H__
#define __STOMP_MANAGEMENT_H__

#include <newt/stomp.h>
#include <pthread.h>

#define DESTINATION_IS_SET (1 << 0)
#define ID_IS_SET          (1 << 1)

typedef struct stomp_msginfo_t {
  char qname[LD_MAX];
  char id[LD_MAX];
  int sock;
  struct list_head list;
  int status;
} stomp_msginfo_t;

typedef struct subscribe_t {
  char id[LD_MAX];
  pthread_t thread_id;
  struct list_head list;
} subscribe_t;

typedef struct management_t {
  pthread_mutex_t mutex;

  struct list_head h_subscribe;
} management_t;

void *stomp_management_worker(void *data);
int iterate_header(struct list_head *, stomp_header_handler_t *, void *);

stomp_msginfo_t *alloc_msginfo();
void free_msginfo(stomp_msginfo_t *);

int initialize_manager();
int register_subscriber(char *, pthread_t);
int unregister_subscriber(char *);
subscribe_t *get_subscriber(char *);

/* processing handlers for each STOMP protocol frames */
frame_t *handler_stomp_connect(frame_t *);
frame_t *handler_stomp_send(frame_t *);
frame_t *handler_stomp_subscribe(frame_t *);
frame_t *handler_stomp_unsubscribe(frame_t *);
frame_t *handler_stomp_ack(frame_t *);
frame_t *handler_stomp_nack(frame_t *);
frame_t *handler_stomp_begin(frame_t *);

#endif
