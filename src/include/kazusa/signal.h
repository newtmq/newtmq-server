#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include <kazusa/list.h>

typedef struct sighandle_t {
  int (*func)(void *);
  void *argument;
  struct list_head list;
} sighandle_t;

int set_signal_handler(int (*)(void *), void *);
void init_signal_handler();

#endif
