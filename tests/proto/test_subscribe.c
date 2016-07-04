#include <test.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#include <newt/common.h>

#include "client.h"
#include "daemon.h"
#include "connection.h"

static void test_subscribe(void) {
  char buf[512];
  char *msg[] = {
    "SUBSCRIBE\n",
    "destination:/queue/test\n",
    "content-length:0\n",
    "id:test1\n",
    "\n",
    NULL,
  };
  int sock;
  char *headers[] = {
    "destination:/queue/test\n",
  };

  sock = connect_server();

  // success to connect with server
  CU_ASSERT(stomp_connect(sock) == RET_SUCCESS);

  // success to send test message
  CU_ASSERT(stomp_send(sock, "hoge\n", 5, headers, 1) == RET_SUCCESS);

  int i;
  for(i=0; msg[i] != NULL; i++) {
    send(sock, msg[i], strlen(msg[i]), 0);
  }
  send(sock, "\0", 1, 0);

  int len = recv(sock, buf, sizeof(buf), 0);
  CU_ASSERT(len > 0);
  CU_ASSERT(strncmp(buf, "MESSAGE\n", 8) == 0);

  close(sock);
}

int test_proto_subscribe(CU_pSuite suite) {
  suite = CU_add_suite("TestSubscribing", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "successful to receive message", test_subscribe);

  return CU_SUCCESS;
}
