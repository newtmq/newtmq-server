#include "unit.h"

#include <newt/config.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void test_load_config(void) {
  char confpath[512];
  newt_config config;

  /* set configuration path */
  assert(getcwd(confpath, sizeof(confpath)) != NULL);
  sprintf(confpath, "%s/%s", confpath, TEST_CONFIG);

  load_config(confpath, &config);

  CU_ASSERT(config.loglevel == NULL);

  // check whether specified parameter is set.
  CU_ASSERT(strcmp(config.server, "testnode") == 0);

  // check whether default parameter is set.
  CU_ASSERT(config.debug == cfg_false);
}

int test_config(CU_pSuite suite) {
  suite = CU_add_suite("ConfigTest", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "check_load_config", test_load_config);

  return CU_SUCCESS;
}
