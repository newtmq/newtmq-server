#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include <newt/list.h>

typedef struct sighandle_t {
  int (*func)(void *);
  void *argument;
  struct list_head list;
} sighandle_t;

sighandle_t *set_signal_handler(int (*)(void *), void *);
int del_signal_handler(sighandle_t *);
void init_signal_handler();

#endif
