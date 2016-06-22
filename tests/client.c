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
    if(send(sock, msg[i], strlen(msg[i]), 0) <= 0) {
      return RET_ERROR;
    }
  }
  send(sock, "\0", 1, 0);

  struct timeval tv;
  tv.tv_sec = RECV_TIMEOUT;
  tv.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

  int ret = RET_ERROR;
  while(recv(sock, buf, sizeof(buf), 0) > 0) {
    if(strncmp(buf, "CONNECTED\n", 10) == 0) {
      ret = RET_SUCCESS;
    }
  }

  return ret;
}

int stomp_send(int sock, char *data, int len) {
  char hdr_buf[LD_MAX];
  int i, hdrlen;

  // send command
  send(sock, "SEND\n", 5, 0);

  // send headers
  hdrlen = sprintf(hdr_buf, "destination:/queue/test\n");
  hdr_buf[hdrlen] = '\0';
  send(sock, hdr_buf, strlen(hdr_buf), 0);

  hdrlen = sprintf(hdr_buf, "content-length:%d\n", len);
  hdr_buf[hdrlen] = '\0';
  send(sock, hdr_buf, strlen(hdr_buf), 0);

  send(sock, "\n", 1, 0);

  // send data
  if(data != NULL) {
    if(send(sock, data, len, 0) <= 0) {
      return RET_ERROR;
    }
  }
  send(sock, "\0", 1, 0);

  return RET_SUCCESS;
}
