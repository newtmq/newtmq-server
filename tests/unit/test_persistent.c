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

#include <persistent_worker.c>

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
  CU_ASSERT(start_persistent_worker() == RET_SUCCESS);

  CU_ASSERT_FATAL(stat(config.datadir, &st) == 0);
  CU_ASSERT(S_ISDIR(st.st_mode));
}

static void check_persistent() {
  frame_t *frame = alloc_frame();
  struct stat st = {0};
  char datadir[512] = {0};

  CU_ASSERT_FATAL(frame != NULL);

  // set test data to a frame
  strncpy(frame->name, "SEND", 4);
  stomp_setdata("content-length:4", 16, &frame->h_attrs, NULL);
  stomp_setdata("hoge:fuga",        9, &frame->h_attrs, NULL);
  stomp_setdata("abcd",             4, &frame->h_data, NULL);
  frame->size = 38; // 5(SEND\n) + (17 + 10)(headers) + 1(separation) + 5(body)

  CU_ASSERT(persistent(QNAME, frame) == RET_SUCCESS);

  // delay to flush data persistently
  sleep(1);

  sprintf(datadir, "%s/%s/data", config.datadir, QNAME);

  CU_ASSERT(stat(datadir, &st) == 0);
  CU_ASSERT(st.st_size == frame->size);
}

static void check_unpersistent() {
  char dirpath[512] = {0};
  struct list_head frame_head;
  struct frame_info *finfo;
  frame_t *frame;

  INIT_LIST_HEAD(&frame_head);

  sprintf(dirpath, "%s/%s", config.datadir, QNAME);

  // unpersist frame
  CU_ASSERT(unpersist_queue_context(dirpath, &frame_head) == RET_SUCCESS);
  CU_ASSERT_FATAL(! list_empty(&frame_head));

  finfo = list_first_entry(&frame_head, struct frame_info, list);
  CU_ASSERT_FATAL(finfo != NULL);

  // check unpersistent frame
  frame = finfo->frame;
  CU_ASSERT_FATAL(frame != NULL);
  CU_ASSERT(! list_empty(&frame->h_attrs));
  CU_ASSERT(! list_empty(&frame->h_data));
  CU_ASSERT(frame->size == 38);

  free_frame_info(finfo);

  // update sent index
  CU_ASSERT(update_index_sent(QNAME, frame) == RET_SUCCESS);

  free_frame(frame);

  // waiting that sent index is updated
  while(unpersist_queue_info(QNAME) > 0L) {
    pthread_yield();
  }

  // unpersist again
  CU_ASSERT(unpersist_queue_context(dirpath, &frame_head) == RET_SUCCESS);
  CU_ASSERT_FATAL(list_empty(&frame_head));
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
  CU_add_test(suite, "check_unpersistent", check_unpersistent);
  CU_add_test(suite, "check_cleanup", check_cleanup);

  return CU_SUCCESS;
}
