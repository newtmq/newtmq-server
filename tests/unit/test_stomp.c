#include "unit.h"

#include <newt/common.h>
#include <newt/stomp_management_worker.h>

#include <stomp.c>

#include <string.h>

#define STDOUT 1
#define BUFLEN 64

static void test_initialize(void) {
  char *inputs[] = { "CONNECT\n", \
                     "accept-version:1.2\n", \
                     "login:guest\n", \
                     "passcode:guest\n", \
                     "\n", \
                     "\0", \
                     NULL };
  struct list_head *e;
  stomp_conninfo_t *c_info;

  CU_ASSERT(stomp_init() == RET_SUCCESS);

  c_info = (stomp_conninfo_t *)conn_init();
  CU_ASSERT(c_info != NULL);

  int i;
  char buf[BUFLEN];
  for(i=0; inputs[i]!=NULL; i++) {
    /* This processing is needed only in test because 'strtox' which is called
     * in stomp_recv_data inhibit to detect string literal in arguments. */
    memset(buf, 0, BUFLEN);
    memcpy(buf, inputs[i], strlen(inputs[i]));

    CU_ASSERT(recv_data(buf, strlen(buf), STDOUT, c_info) == RET_SUCCESS);

    if(strcmp(inputs[i], "\n") == 0 || strcmp(inputs[i], "\0") == 0) {
      CU_ASSERT(c_info->frame == NULL);
    } else {
      CU_ASSERT(c_info->frame != NULL);
    }
  }
}

static void test_bucket(void) {
  struct list_head *e;
  int frame_count = 0;

  list_for_each(e, &stomp_frame_bucket.h_frame) {
    frame_count++;
  }
  CU_ASSERT(frame_count == 1);
}

static void test_frame(void) {
  frame_t *frame;

  list_for_each_entry(frame, &stomp_frame_bucket.h_frame, l_bucket) {
    struct list_head *e;
    int frame_count;

    /* check attrs in target frame */
    frame_count = 0;
    list_for_each(e, &frame->h_attrs) {
      frame_count++;
    }
    CU_ASSERT(frame_count == 3);
  }
}

static void test_subscriber(void) {
  char id[] = "hoge";
  pthread_t tid;

  CU_ASSERT(initialize_manager() == RET_SUCCESS);
  CU_ASSERT(register_subscriber(id, tid) == RET_SUCCESS);
  CU_ASSERT(get_subscriber(id) != NULL);
  CU_ASSERT(unregister_subscriber(id) == RET_SUCCESS);
}

int test_stomp(CU_pSuite suite) {
  suite = CU_add_suite("STOMP Test", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "start_connection and create a frame", test_initialize);
  CU_add_test(suite, "check bucket", test_bucket);
  CU_add_test(suite, "check frame in bucket", test_frame);
  CU_add_test(suite, "check subscriber", test_subscriber);

  return CU_SUCCESS;
}
