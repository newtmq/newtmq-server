#include <test.h>

#include <string.h>
#include <pthread.h>

#include <newt/common.h>

#include "client.h"
#include "connection.h"

#define BUFLEN 512

static void *subscriber(void *data) {
  char buf[BUFLEN] = {0};
  char *msg[] = {
    "SUBSCRIBE\n",
    "destination:/queue/test\n",
    "content-length:0\n",
    "id:test1\n",
    "\n",
    NULL,
  };
  int sock = connect_server();

  int i;
  for(i=0; msg[i] != NULL; i++) {
    test_send(sock, msg[i], strlen(msg[i]));
  }
  test_send(sock, "\0", 1);

  // receive MESSAGE frame
  char *reply_dest = NULL;
  while(test_recv(sock, buf, sizeof(buf)) > 0) {
    char *line;
    line = strtok(buf, "\n");
    do{
      if(line != NULL && (strlen(line) > 9) && (strncmp(line, "reply-to:", 9) == 0)) {
        reply_dest = malloc(BUFLEN);
        sprintf(reply_dest, "destination:%s\n", (line+9));
      }
    } while((line = strtok(NULL, "\n")) != NULL);
    memset(buf, 0, sizeof(buf));
  }
  CU_ASSERT(reply_dest != NULL);

  if(reply_dest != NULL) {
    char *reply_headers[] = {
      reply_dest,
    };
    stomp_send(sock, "puyo\n", 5, reply_headers, 1);

    free(reply_dest);
  }

  close(sock);

  return NULL;
}

static void test_reply_to(void) {
  char buf[BUFLEN] = {0};
  char *additional_headers[] = {
    "destination:/queue/test\n",
    "reply-to:/temp-queue/test\n",
  };
  int sock;

  pthread_t thread_id;

  sock = connect_server();

  // success to connect with server
  CU_ASSERT(stomp_connect(sock) == RET_SUCCESS);

  // success to send test message
  CU_ASSERT(stomp_send(sock, "hoge\n", 5, additional_headers, 2) == RET_SUCCESS);

  pthread_create(&thread_id, NULL, &subscriber, NULL);
  pthread_join(thread_id, NULL);

  // receive replied MESSAGE frame
  CU_ASSERT(test_recv(sock, buf, sizeof(buf)) > 0);
  CU_ASSERT(strncmp(buf, "MESSAGE\n", 8) == 0);

  close(sock);
}

int test_proto_reply_to(CU_pSuite suite) {
  suite = CU_add_suite("TestReply", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "successful to reply message", test_reply_to);

  return CU_SUCCESS;
}
