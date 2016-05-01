#include "connection.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int open_connection(int port) {
  int sd;
  struct sockaddr_in addr;
 
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }
 
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
 
  connect(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

  return sd;
}
