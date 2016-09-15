#include <newt/config.h>
#include <newt/common.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <test.h>
#include <string.h>

#include "client.h"
#include "config.h"
#include "connection.h"

int connect_server() {
  newt_config config = {0};
  int sock = -1;

  set_config(&config);
  if(config.port > 0) {
    sock = open_connection(config.port);
  }

  return sock;
}

int connect_ctrl_server() {
  newt_config config = {0};
  int sock = -1;

  set_config(&config);
  if(config.port > 0) {
    sock = open_connection(config.ctrl_port);
  }

  return sock;
}

int stomp_connect(int sock) {
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

  int i;
  for(i=0; msg[i] != NULL; i++) {
    if(test_send(sock, msg[i], strlen(msg[i]), 0) <= 0) {
      return RET_ERROR;
    }
  }
  test_send(sock, "\0", 1, 0);

  int ret = RET_ERROR;
  while(recv(sock, buf, sizeof(buf), 0) > 0) {
    if(strncmp(buf, "CONNECTED\n", 10) == 0) {
      ret = RET_SUCCESS;
    }
  }

  return ret;
}

int stomp_send(int sock, char *data, int len, char **headers, int header_len) {
  char hdr_buf[LD_MAX];
  int i, hdrlen;

  // send command
  test_send(sock, "SEND\n", 5, 0);

  // send headers
  for(i=0; i<header_len; i++) {
    test_send(sock, headers[i], strlen(headers[i]), 0);
  }

  hdrlen = sprintf(hdr_buf, "content-length:%d\n", len);
  hdr_buf[hdrlen] = '\0';
  test_send(sock, hdr_buf, strlen(hdr_buf), 0);

  test_send(sock, "\n", 1, 0);

  // send data
  if(data != NULL) {
    if(test_send(sock, data, len, 0) <= 0) {
      return RET_ERROR;
    }
  }
  test_send(sock, "\0", 1, 0);

  return RET_SUCCESS;
}
