#include <test.h>
#include <sys/socket.h>

#include <newt/newtctl.h>
#include <newtctl.c>

#include "client.h"

#define BUFSIZE (1<<13)

static void test_ctrl_handler(void) {
  newtctl_t obj = {NEWTCTL_LIST_QUEUES};
  int sock;
  int datalen = 0;
  char *encoded_data, *recvbuf;

  recvbuf = (char *)malloc(BUFSIZE);

  CU_ASSERT(handlers != NULL);

  encoded_data = pack(&obj, &datalen);

  CU_ASSERT(encoded_data != NULL);
  CU_ASSERT(datalen > 0);

  sock = connect_ctrl_server();
  CU_ASSERT(sock > 0);

  CU_ASSERT(send(sock, encoded_data, datalen, 0) > 0);

  int len = recv(sock, recvbuf, BUFSIZE, 0);
  CU_ASSERT(len > 0);

  CU_ASSERT(unpack(recvbuf, len, &obj) == RET_SUCCESS);
  CU_ASSERT(obj.status == RET_SUCCESS);

  free(encoded_data);
  free(recvbuf);
  close(sock);
}

int test_newtctl_worker(CU_pSuite suite) {
  suite = CU_add_suite("TestNewtctlWorker", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "succeed in calling handler", test_ctrl_handler);

  return CU_SUCCESS;
}
