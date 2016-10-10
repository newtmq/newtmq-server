#include "connection.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>

#include <newt/logger.h>

int open_connection(int port) {
  int sd;
  struct sockaddr_in addr;
  struct timeval tv;

  tv.tv_sec = RECV_TIMEOUT;
  tv.tv_usec = 0;
 
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }
 
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
 
  connect(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

  setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

  return sd;
}

int test_send(int sock, char *buf, int len) {
  int ret;

  ret = send(sock, buf, len, 0);
  if(ret < 0) {
    printf("[test_send/ERROR] (%d) %s [%d]\n", errno, buf, len);
  }

  return ret;
}

int test_recv(int sock, char *buf, int len) {
  char received_data;
  int index = 0;

  while(recv(sock, &received_data, 1, 0) > 0 && index < len) {
    buf[index++] = received_data;

    if(received_data == '\0') {
      break;
    }
  }

  return index;
}
