#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <kazusa/signal.h>
#include <kazusa/common.h>

LIST_HEAD(h_signal);

static void cleanup_handler() {
  sighandle_t *handler, *h;

  list_for_each_entry_safe(handler, h, &h_signal, list) {
    free(handler);
  }
}

static void int_handle(int code) {
  sighandle_t *handler;

  list_for_each_entry(handler, &h_signal, list) {
    (*handler->func)(handler->argument);
  }
  cleanup_handler();

  exit(0);
}

int set_signal_handler(int (*func)(void *), void *arg) {
  sighandle_t *handler;

  handler = (sighandle_t *)malloc(sizeof(sighandle_t));
  if(handler == NULL) {
    return RET_ERROR;
  }
  memset(handler, 0, sizeof(sighandle_t));
  handler->func = func;
  handler->argument = arg;

  list_add(&handler->list, &h_signal);

  return RET_SUCCESS;
}

void init_signal_handler() {
  signal(SIGINT, int_handle);
}
