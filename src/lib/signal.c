#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <newt/signal.h>
#include <newt/common.h>

LIST_HEAD(h_signal);

static void cleanup_handler() {
  sighandle_t *handler, *h;

  list_for_each_entry_safe(handler, h, &h_signal, list) {
    list_del(&handler->list);
    free(handler);
  }
}

static void int_handle(int code) {
  sighandle_t *handler;

  info("An interrupt signal has received. NewtMQ will be stopped after cleanup processing.");

  list_for_each_entry(handler, &h_signal, list) {
    (*handler->func)(handler->argument);
  }
  cleanup_handler();

  exit(0);
}

sighandle_t *set_signal_handler(int (*func)(void *), void *arg) {
  sighandle_t *handler;

  handler = (sighandle_t *)malloc(sizeof(sighandle_t));
  if(handler == NULL) {
    return NULL;
  }
  memset(handler, 0, sizeof(sighandle_t));
  handler->func = func;
  handler->argument = arg;

  list_add(&handler->list, &h_signal);

  return handler;
}

int del_signal_handler(sighandle_t *handler) {
  int ret = RET_ERROR;

  if(handler != NULL) {
    list_del(&handler->list);
    free(handler);

    ret = RET_SUCCESS;
  }

  return ret;
}

void init_signal_handler() {
  signal(SIGINT, int_handle);
}
