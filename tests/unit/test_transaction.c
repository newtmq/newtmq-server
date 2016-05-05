#include "unit.h"

#include <newt/transaction.h>
#include <newt/common.h>
#include <newt/stomp.h>

#include <string.h>
#include <assert.h>

#define TEST_TID "tid-test"

static int callback_test(frame_t *frame) {
  int *counter = (int *)frame->transaction_data;

  CU_ASSERT(frame != NULL);
  CU_ASSERT(counter != NULL);

  *counter = 1;

  return RET_SUCCESS;
}

static void test_init(void) {
  CU_ASSERT(transaction_init() == RET_SUCCESS);
}

static void test_start(void) {
  CU_ASSERT(transaction_start(TEST_TID) == RET_SUCCESS);
}

static void test_add(void) {
  CU_ASSERT(transaction_add(TEST_TID, alloc_frame()) == RET_SUCCESS);
}

static void test_abort() {
  CU_ASSERT(transaction_abort(TEST_TID) == RET_SUCCESS);
}

static void test_commit() {
  frame_t *frame = alloc_frame();
  int *counter = (int *)malloc(sizeof(int));

  assert(counter != NULL);

  *counter = 0;

  strcpy(frame->name, "TEST");
  frame->transaction_callback = callback_test;
  frame->transaction_data = counter;

  transaction_start(TEST_TID);
  transaction_add(TEST_TID, frame);

  CU_ASSERT(transaction_commit(TEST_TID) == RET_SUCCESS);
  CU_ASSERT(*counter > 0);

  free_frame(frame);
}

static void test_destruct() {
  CU_ASSERT(transaction_destruct() == RET_SUCCESS);
}

int test_transaction(CU_pSuite suite) {
  suite = CU_add_suite("TransactionTest", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "check_init", test_init);
  CU_add_test(suite, "check_start", test_start);
  CU_add_test(suite, "check_add", test_add);
  CU_add_test(suite, "check_abort", test_abort);
  CU_add_test(suite, "check_commit", test_commit);
  CU_add_test(suite, "check_destruct", test_destruct);

  return CU_SUCCESS;
}
