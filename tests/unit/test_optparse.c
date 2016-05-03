#include "unit.h"

static void test_set_valid(void) {
  struct cmd_args args = {0};
  char config_path[] = "/usr/local/etc/newtd.conf";
  char *argv[] = {"newtd", "-c", config_path};
  int argc = 3;

  parse_opt(argc, argv, &args);

  CU_ASSERT_STRING_EQUAL(args.config_path, config_path);
}

static void test_set_nothing(void) {
  struct cmd_args args = {0};
  char *argv[] = {"newtd"};
  int argc = 1;

  parse_opt(argc, argv, &args);

  CU_ASSERT_EQUAL(args.config_path, NULL);
}

int test_optparse(CU_pSuite suite) {
  suite = CU_add_suite("OptionParser", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "set_valid_one", test_set_valid);
  CU_add_test(suite, "blank", test_set_nothing);

  return CU_SUCCESS;
}
