#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <pthread.h>

#include <newt/list.h>
#include <newt/common.h>

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
  char name[QNAME_LENGTH];

  // for persistency
  long index_head; // the whole bytes of queue
  long index_sent; // the bytes which is already sent to subscriber in queue

  // for statistics
  int count;
};

int enqueue(void *, char *);
void *dequeue(char *);
int get_queuelist(struct list_head *);

int initialize_queuebox();
int cleanup_queuebox();

#endif
