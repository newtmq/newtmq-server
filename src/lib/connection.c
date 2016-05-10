#include <newt/connection.h>
#include <newt/config.h>
#include <newt/stomp.h>
#include <newt/signal.h>
#include <newt/common.h>
#include <newt/logger.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <pthread.h>

#define RECV_BUFSIZE (4096)
#define QUEUENUM (1 << 10)

/* This data structure is used only in the active connection
 * and exists one object per connection. */
struct conninfo {
  int sock;
  void *protocol_data;
  struct list_head h_buf;
  pthread_mutex_t mutex;
};

static struct conninfo* alloc_conninfo() {
  struct conninfo *cinfo;

  cinfo = (struct conninfo *)malloc(sizeof(struct conninfo));
  if(cinfo != NULL) {
    memset(cinfo, 0, sizeof(struct conninfo));

    INIT_LIST_HEAD(&cinfo->h_buf);
    pthread_mutex_init(&cinfo->mutex, NULL);
  }

  return cinfo;
}

static int cleanup_co_worker(void *data) {
  struct conninfo *cinfo = (struct conninfo *)data;
  int ret = RET_ERROR;

  if(cinfo != NULL) {
    close(cinfo->sock);

    if(cinfo->protocol_data != NULL) {
      free(cinfo->protocol_data);
    }

    free(cinfo);

    ret = RET_SUCCESS;
  }

  return ret;
}

static int cleanup_connection(void *data) {
  int *sock = (int *)data;
  int ret = RET_ERROR;

  if(sock != NULL) {
    close(*sock);

    ret = RET_SUCCESS;
  }
  return ret;
}

static void *connection_co_worker(void *data) {
  struct conninfo *cinfo = (struct conninfo*)data;
  sighandle_t *handler;
  char buf[RECV_BUFSIZE];

  if(cinfo == NULL) {
    err("[connection_co_worker] thread argument is NULL");
    return NULL;
  }

  cinfo->protocol_data = (void *)stomp_conn_init();

  /* initialize processing after established connection */
  handler = set_signal_handler(cleanup_co_worker, &cinfo);

  int len;
  do {
    memset(buf, 0, RECV_BUFSIZE);
    len = recv(cinfo->sock, buf, sizeof(buf), 0);

    stomp_recv_data(buf, len, cinfo->sock, cinfo->protocol_data);
  } while(len > 0);

  // cancel to parse of the current frame
  stomp_conn_finish(cinfo->protocol_data);

  del_signal_handler(handler);

  close(cinfo->sock);

  free(cinfo);

  return NULL;
}

void *connection_worker(void *data) {
  newt_config *conf = (newt_config *)data;
  struct sockaddr_in addr;
  int sd;
 
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return NULL;
  }
 
  addr.sin_family = AF_INET;
  addr.sin_port = htons(conf->port);
  addr.sin_addr.s_addr = INADDR_ANY;
  if(conf->server != NULL) {
    addr.sin_addr.s_addr = inet_addr(conf->server);
  }
 
  if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return NULL;
  }
 
  if(listen(sd, QUEUENUM) < 0) {
    perror("listen");
    return NULL;
  }

  set_signal_handler(cleanup_connection, &sd);

  info("NewtMQ is ready to accept requests (port: %d)", conf->port);
 
  struct sockaddr_in from_addr;
  socklen_t sin_size = sizeof(struct sockaddr_in);
  pthread_t thread_id;
  while(1) {
    int acc_sd;
    struct conninfo *cinfo;

    if((acc_sd = accept(sd, (struct sockaddr *)&from_addr, &sin_size)) < 0) {
      err("[connection_worker] failed to accept connection");
      continue;
    }

    cinfo = alloc_conninfo();
    if(cinfo != NULL) {
      cinfo->sock = acc_sd;

      if(pthread_create(&thread_id, NULL, &connection_co_worker, cinfo)) {
        err("[connection_worker] failed to create connection_co_worker");
      }
    }
  }
 
  return NULL;
}

int send_msg(int sock, char *msg) {
  int ret;

  if(msg != NULL) {
    ret = send(sock, msg, strlen(msg), 0);
  } else {
    ret = send(sock, "\0", 1, 0);
  }

  return ret;
}

int is_socket_valid(int sock) {
  int error = 0;
  socklen_t len = sizeof (error);
  int retval = getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);

  if (retval != 0 || error != 0) {
    return RET_ERROR;
  }

  return RET_SUCCESS;
}
