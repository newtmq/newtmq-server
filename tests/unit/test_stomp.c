#include "unit.h"

#include <kazusa/common.h>

#include <stdio.h>

#define STDOUT 1

static void test_basic_feature(void) {
  void *state_data;
  char recv_data[10] = "hogefuga\n\0";
  int frame_count;
  frame_t *frame;
  struct list_head *e;

  CU_ASSERT(stomp_init_bucket() == RET_SUCCESS);

  state_data = (void *)stomp_init_connection(STDOUT);
  CU_ASSERT_FATAL(state_data != NULL);

  stomp_recv_data(recv_data, 10, state_data);

  frame_count = 0;
  list_for_each_entry(frame, &stomp_frame_bucket.l_frame_h, l_bucket) {
    int attr_count = 0;
    frame_attr_t *attr;
    list_for_each_entry(attr, &frame->l_attrs_h, l_frame) {
      CU_ASSERT(strcmp(attr->data, "hogefuga") == 0);

      attr_count++;
    }
    CU_ASSERT(attr_count == 1);

    frame_count++;
  }
  CU_ASSERT(frame_count == 1);

  CU_ASSERT(stomp_cleanup() == RET_SUCCESS);

  frame_count = 0;
  list_for_each(e, &stomp_frame_bucket.l_frame_h) {
    frame_count++;
  }
  CU_ASSERT(frame_count == 0);
}

int test_stomp(CU_pSuite suite) {
  suite = CU_add_suite("STOMP Test", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "test_basic_feature", test_basic_feature);

  return CU_SUCCESS;
}
