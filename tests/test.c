#include "unit/unit.h"

#define ADD_TESTS(statement) ret = statement(suite); if(ret != CUE_SUCCESS) return ret;

int main(int argc, char **argv) {
  CU_pSuite suite;
  int ret;

  CU_initialize_registry();

  ADD_TESTS(test_optparse);
  ADD_TESTS(test_config);
  ADD_TESTS(test_stomp);
  ADD_TESTS(test_signal);
  ADD_TESTS(test_daemon);

  CU_basic_run_tests();
  CU_cleanup_registry();

  return 0;
}
