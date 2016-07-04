#include <test.h>

#include <string.h>
#include <pthread.h>

#include <newt/common.h>

#include "client.h"
#include "connection.h"

#define BUFLEN 512
#define THREAD_NUM 2
#define RETRY_MAX 10

static void *subscriber(void *data) {
  char buf[BUFLEN] = {0};
  char *msg[] = {
    "SUBSCRIBE\n",
    "destination:/topic/test\n",
    "content-length:0\n",
    "\n",
    NULL,
  };
  int sock, i;

  sock = connect_server();

  for(i=0; msg[i]!=NULL; i++) {
    mysend(sock, msg[i], strlen(msg[i]), 0);
  }
  mysend(sock, "\0", 1, 0);

  int len;
  int retry_count = RETRY_MAX;
  do {
    len = recv(sock, buf, sizeof(buf), 0);
    retry_count--;
  } while(retry_count > 0 && len < 0);

  CU_ASSERT(len > 0);
  CU_ASSERT(strncmp(buf, "MESSAGE", 7) == 0);

  close(sock);

  return NULL;
}

static void test_topic(void) {
  char buf[BUFLEN] = {0};
  char *additional_headers[] = {
    "destination:/topic/test\n",
  };
  pthread_t threads[THREAD_NUM];
  int sock, i;

  for(i=0; i<THREAD_NUM; i++) {
    pthread_create(&threads[i], NULL, &subscriber, NULL);
  }

  sock = connect_server();

  // success to connect with server
  CU_ASSERT(stomp_connect(sock) == RET_SUCCESS);
  CU_ASSERT(stomp_send(sock, "hoge\n", 5, additional_headers, 1) == RET_SUCCESS);

  for(i=0; i<THREAD_NUM; i++) {
    pthread_join(threads[i], NULL);
  }

  close(sock);
}

int test_proto_topic(CU_pSuite suite) {
  suite = CU_add_suite("TestTopic", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "successful to topic pub/sub", test_topic);

  return CU_SUCCESS;
}
