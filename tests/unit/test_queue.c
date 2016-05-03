#include "unit.h"

#include <newt/common.h>
#include <newt/queue.h>

#include <stdlib.h>

#define DATALEN 10000
#define QNAMELEN 128

struct test_data {
  char qname[QNAMELEN];
  int value;
};
struct test_data data_arr[DATALEN];

static void gen_random(char *s, const int len) {
  static const char alphanum[] = 
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  int i;
  
  for (i=0; i<len; i++) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }
  
  s[len] = 0;
}

static void check_init(void) {
  CU_ASSERT(initialize_queuebox() == RET_SUCCESS);
}

static void check_enqueue(void) {
  int i;
  int ret = RET_ERROR;
  struct test_data *data;

  for(i=0; i<DATALEN; i++) {
    data = &data_arr[i];

    /* initialize data */
    data->value = i;
    gen_random(data->qname, QNAMELEN);

    ret = enqueue((void *)data, data->qname);
    if(ret == RET_ERROR) {
      break;
    }
  }

  CU_ASSERT(ret == RET_SUCCESS);
}

static void check_dequeue(void) {
  int i;
  int ret = RET_SUCCESS;
  struct test_data *data, *get_data;

  for(i=0; i<DATALEN; i++) {
    data = &data_arr[i];

    get_data = (struct test_data *)dequeue(data->qname);
    if(get_data != data) {
      ret = RET_ERROR;
      break;
    }
  }

  CU_ASSERT(ret == RET_SUCCESS);
}

static void check_cleanup(void) {
  CU_ASSERT(cleanup_queuebox() == RET_SUCCESS);
}

int test_queue(CU_pSuite suite) {
  suite = CU_add_suite("Test of queue feature", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "check queue initialization", check_init);
  CU_add_test(suite, "check entier object to queue", check_enqueue);
  CU_add_test(suite, "check get object from queue", check_dequeue);
  CU_add_test(suite, "check cleanup processing about queue", check_cleanup);

  return CU_SUCCESS;
}
