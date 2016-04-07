#include "unit.h"

#include <kazusa/common.h>

#include <stdio.h>

#define STDOUT 1
#define BUFLEN 64

static void test_initialize(void) {
  char *inputs[] = { "CONNECT\n", \
                     "accept-version:1.2\n", \
                     "login:guest\n", \
                     "passcode:guest\n", \
                     "\n", \
                     "hogefuga\n", \
                     "foobar\n", \
                     "\0", \
                     NULL };
  frame_t *frame = NULL;
  struct list_head *e;

  CU_ASSERT(stomp_init_bucket() == RET_SUCCESS);

  int i;
  char buf[BUFLEN];
  for(i=0; inputs[i]!=NULL; i++) {
    /* This processing is needed only in test because 'strtox' which is called
     * in stomp_recv_data inhibit to detect string literal in arguments. */
    memset(buf, 0, BUFLEN);
    memcpy(buf, inputs[i], strlen(inputs[i]));

    CU_ASSERT(stomp_recv_data(buf, strlen(buf), STDOUT, (void **)&frame) == RET_SUCCESS);
    CU_ASSERT(frame != NULL);
  }

  CU_ASSERT(strcmp(frame->name, "CONNECT") == 0);
  CU_ASSERT(GET_STATUS(frame, STATUS_IN_BUCKET));
}

static void test_management(void) {
  struct list_head *e;
  int frame_count = 0;

  list_for_each(e, &stomp_frame_bucket.h_frame) {
    frame_count++;
  }
  CU_ASSERT(frame_count == 1);
}

static void test_cleanup(void) {
  struct list_head *e;
  int frame_count = 0;

  stomp_cleanup();
  list_for_each(e, &stomp_frame_bucket.h_frame) {
    frame_count++;
  }
  CU_ASSERT(frame_count == 0);
}

int test_stomp(CU_pSuite suite) {
  suite = CU_add_suite("STOMP Test", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "start_connection and create a frame", test_initialize);
  CU_add_test(suite, "check management data-structure", test_management);
  CU_add_test(suite, "check cleanup processing", test_cleanup);

  return CU_SUCCESS;
}
