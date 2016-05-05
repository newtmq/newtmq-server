#ifndef __TRANSACTION_H__
#define __TRANSACTION_H__

#include <newt/list.h>
#include <newt/stomp.h>

#include <pthread.h>

typedef struct transaction_t {
  char tid[512];
  struct list_head l_manager;
  struct list_head h_frame;
  pthread_mutex_t mutex;
} transaction_t;

int transaction_init();
int transaction_destruct();
int transaction_start(char *);
int transaction_add(char *, frame_t *);
int transaction_abort(char *);
int transaction_commit(char *);

#endif
