#include <test.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>

#include "client.h"
#include "daemon.h"

static void test_connect(void) {
  char buf[512];
  char *msg[] = {
    "CONNECT\n",
    "content-length:0\n",
    "accept-version:1.2\n",
    "login:guest\n",
    "passcode:guest\n",
    "\n",
    NULL,
  };
  int sock, len, i;

  sock = connect_server();
  CU_ASSERT(sock > 0);

  for(i=0; msg[i] != NULL; i++) {
    len = send(sock, msg[i], strlen(msg[i]), 0);
    assert(len > 0);
  }
  CU_ASSERT(send(sock, "\0", 1, 0) > 0);

  len = test_recv(sock, buf, sizeof(buf));
  CU_ASSERT(len > 0);

  CU_ASSERT(strncmp(buf, "CONNECTED\n", 10) == 0);

  CU_ASSERT(close(sock) == 0);
}

int test_proto_connect(CU_pSuite suite) {
  suite = CU_add_suite("TestConnectFrame", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "succeed in sending CONNECT frame", test_connect);

  return CU_SUCCESS;
}
