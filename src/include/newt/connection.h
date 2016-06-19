#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <newt/config.h>
#include <newt/list.h>

void *ctrl_connection_worker(void *);
void *connection_worker(void *);
int send_msg(int, char *, int);
int is_socket_valid(int);

/* This data structure is used only in the active connection
 * and exists one object per connection. */
struct conninfo {
  int sock;
  void *protocol_data;
  struct list_head h_buf;
  pthread_mutex_t mutex;
  void *(*handler)(struct conninfo *);
};

#endif
