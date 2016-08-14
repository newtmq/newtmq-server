#define _XOPEN_SOURCE 500
#include "unit.h"

#include <newt/common.h>
#include <newt/config.h>
#include <newt/frame.h>
#include <newt/persistent_worker.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <ftw.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#define QNAME "/queue/unit-test"

static newt_config config;

static int do_delete(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
  switch (tflag) {
    case FTW_D:
    case FTW_DNR:
    case FTW_DP:
      rmdir (fpath);
      break;
    default:
      unlink (fpath);
      break;
  }
  return 0;
}

static void clear_queue_data(char *dirpath) {
  nftw(dirpath, do_delete, 20, FTW_DEPTH);
}

static void check_initialize() {
  char confpath[512];
  struct stat st = {0};

  assert(getcwd(confpath, sizeof(confpath)) != NULL);
  sprintf(confpath, "%s/%s", confpath, TEST_CONFIG);

  load_config(confpath, &config);

  clear_queue_data(config.datadir);

  CU_ASSERT(initialize_persistent_worker(&config) == RET_SUCCESS);

  CU_ASSERT_FATAL(stat(config.datadir, &st) == 0);
  CU_ASSERT(S_ISDIR(st.st_mode));
}

static void check_persistent() {
  frame_t *frame = alloc_frame();
  struct stat st = {0};
  char datadir[512] = {0};

  CU_ASSERT_FATAL(frame != NULL);

  // set test data to a frame
  strncpy(frame->name, "TEST", 4);
  stomp_setdata("foo:bar",    7, &frame->h_attrs, NULL);
  stomp_setdata("hoge:fuga",  9, &frame->h_attrs, NULL);
  stomp_setdata("abcd",       4, &frame->h_data, NULL);
  frame->size = 5 + 8 + 10 + 1 + 5;

  CU_ASSERT(persistent(QNAME, frame) == RET_SUCCESS);

  // delay to flush data persistently
  sleep(1);

  sprintf(datadir, "%s/%s/data", config.datadir, QNAME);

  CU_ASSERT(stat(datadir, &st) == 0);
  CU_ASSERT(st.st_size == frame->size);
}

static void check_cleanup() {
  CU_ASSERT(cleanup_persistent_worker() == RET_SUCCESS);
}

int test_persistent(CU_pSuite suite) {
  suite = CU_add_suite("PersistentQueueData", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "check_initialize", check_initialize);
  CU_add_test(suite, "check_persistent", check_persistent);
  CU_add_test(suite, "check_cleanup", check_cleanup);

  return CU_SUCCESS;
}
