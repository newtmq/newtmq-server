#include <test.h>

#include <sys/socket.h>
#include <string.h>
#include <assert.h>

#include "client.h"
#include "daemon.h"

static void test_begin(void) {
  char buf[512];
  char *msg[] = {
    "BEGIN\n",
    "transaction:tx1\n",
    NULL,
  };
  int sock, len, i;

  sock = connect_server();
  assert(sock > 0);

  for(i=0; msg[i] != NULL; i++) {
    len = send(sock, msg[i], strlen(msg[i]), 0);
    assert(len > 0);
  }
  send(sock, "\0", 1, 0);

  sleep(0.5);

  /* check not to receive ERROR frame */
  len = recv(sock, buf, sizeof(buf), MSG_DONTWAIT);
  CU_ASSERT(len < 0);

  close(sock);
}

int test_proto_begin(CU_pSuite suite) {
  suite = CU_add_suite("TestConnectFrame", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "succeed in sending BEGIN frame", test_begin);

  return CU_SUCCESS;
}
