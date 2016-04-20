#ifndef __STOMP_MESSAGE_WORKER_H__
#define __STOMP_MESSAGE_WORKER_H__

typedef struct stomp_msginfo_t {
  char qname[LD_MAX];
  int sock;
  struct list_head list;
} stomp_msginfo_t;

void stomp_message_worker_init();
void *stomp_message_worker(void *);
int stomp_message_register(stomp_msginfo_t *);
stomp_msginfo_t *alloc_msginfo();
void free_msginfo(stomp_msginfo_t *);

#endif
