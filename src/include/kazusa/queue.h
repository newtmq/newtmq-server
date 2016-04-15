#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <pthread.h>

#define QB_SIZE 512

#define FLAG_DELETE_DATA (1 << 0)

struct q_entry {
  struct list_head l_queue;
  void *data;
};

struct queue {
  pthread_mutex_t mutex;
  unsigned long hashnum;
  struct list_head l_box;
  struct list_head h_entry;
};

int enqueue(void *, char *);
void *dequeue(char *);

int initialize_queuebox();
int cleanup_queuebox();

#endif
