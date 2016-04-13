#include "unit.h"

#include <kazusa/common.h>

#include <string.h>

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

  CU_ASSERT(stomp_init() == RET_SUCCESS);


  int i;
  char buf[BUFLEN];
  for(i=0; inputs[i]!=NULL; i++) {
    /* This processing is needed only in test because 'strtox' which is called
     * in stomp_recv_data inhibit to detect string literal in arguments. */
    memset(buf, 0, BUFLEN);
    memcpy(buf, inputs[i], strlen(inputs[i]));

    CU_ASSERT(stomp_recv_data(buf, strlen(buf), STDOUT, (void **)&frame) == RET_SUCCESS);

    if(strncmp(inputs[i], "\0", 1) == 0) {
      CU_ASSERT(frame == NULL);
    } else {
      CU_ASSERT(frame != NULL);
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

    /* check data lines in target frame */
    frame_count = 0;
    list_for_each(e, &frame->h_data) {
      frame_count++;
    }
    CU_ASSERT(frame_count == 2);
  }
}

int test_stomp(CU_pSuite suite) {
  suite = CU_add_suite("STOMP Test", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "start_connection and create a frame", test_initialize);
  CU_add_test(suite, "check bucket", test_bucket);
  CU_add_test(suite, "check frame in bucket", test_frame);

  return CU_SUCCESS;
}
