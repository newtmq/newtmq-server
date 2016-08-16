#include "test.h"
#include "daemon.h"

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#define ADD_TESTS(statement) ret = statement(suite); if(ret != CUE_SUCCESS) return ret;

static void test_init() {
  init_config();
}

int main(int argc, char **argv) {
  CU_pSuite suite;
  int ret, failed_num;
  pid_t pid;

  signal(SIGPIPE, SIG_IGN);

  // preprocessing of tests
  test_init();

  pid = start_newtd();
  if(pid > 0) {
    CU_initialize_registry();

    ADD_TESTS(test_optparse);
    ADD_TESTS(test_config);
    ADD_TESTS(test_signal);
    ADD_TESTS(test_daemon);
    ADD_TESTS(test_frame);
    ADD_TESTS(test_stomp);
    ADD_TESTS(test_queue);
    ADD_TESTS(test_transaction);
    ADD_TESTS(test_persistent);
    ADD_TESTS(test_proto_connect);
    ADD_TESTS(test_proto_disconnect);
    ADD_TESTS(test_proto_subscribe);
    ADD_TESTS(test_proto_reply_to);
    ADD_TESTS(test_proto_topic);
    ADD_TESTS(test_proto_begin);
    ADD_TESTS(test_proto_error);
    ADD_TESTS(test_newtctl_worker);

    CU_basic_run_tests();

    failed_num = CU_get_number_of_tests_failed();

    CU_cleanup_registry();


    kill(pid, SIGINT);
    wait(NULL);
  }

  return failed_num;
}
