#include "unit.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>

#include <newt/signal.h>
#include <newt/common.h>

static int handler_test1(void *data) {
  (*(int *)data)++;
  return 0;
}

static int handler_test2(void *data) {
  (*(int *)data)++;
  return 0;
}

static void check_signal(void) {
  pid_t pid;
  int *data = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  /* initialize data */
  *data = 0;

  if((pid = fork()) == 0) {
    init_signal_handler();

    CU_ASSERT(set_signal_handler(handler_test1, data) != NULL);
    CU_ASSERT(set_signal_handler(handler_test2, data) != NULL);

    while(1);
  } else {
    sleep(1);

    kill(pid, SIGINT);
    wait(NULL);

  }
  CU_ASSERT(*data == 2);

  munmap(data, sizeof(int));
}

int test_signal(CU_pSuite suite) {
  suite = CU_add_suite("SignalTest", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "check signal handler", check_signal);

  return CU_SUCCESS;
}
