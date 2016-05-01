#include "test.h"
#include "daemon.h"

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#define ADD_TESTS(statement) ret = statement(suite); if(ret != CUE_SUCCESS) return ret;

int main(int argc, char **argv) {
  CU_pSuite suite;
  int ret;
  pid_t pid;

  pid = start_kazusad();
  if(pid > 0) {
    CU_initialize_registry();

    ADD_TESTS(test_optparse);
    ADD_TESTS(test_config);
    ADD_TESTS(test_signal);
    ADD_TESTS(test_daemon);
    ADD_TESTS(test_stomp);
    ADD_TESTS(test_queue);
    ADD_TESTS(test_proto_connect);

    CU_basic_run_tests();
    CU_cleanup_registry();

    kill(pid, SIGINT);
    wait(NULL);
  }

  return 0;
}
