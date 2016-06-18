#include <newt/connection.h>
#include <newt/config.h>
#include <newt/stomp.h>
#include <newt/newtctl.h>
#include <newt/signal.h>
#include <newt/common.h>
#include <newt/logger.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <pthread.h>
#include <assert.h>

#define QUEUENUM (1 << 10)

static struct conninfo* alloc_conninfo() {
  struct conninfo *cinfo;

  cinfo = (struct conninfo *)malloc(sizeof(struct conninfo));
  if(cinfo != NULL) {
    memset(cinfo, 0, sizeof(struct conninfo));

    INIT_LIST_HEAD(&cinfo->h_buf);
    pthread_mutex_init(&cinfo->mutex, NULL);
    cinfo->handler = NULL;
  }

  return cinfo;
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

static int init_connection(char *host, int port) {
  struct sockaddr_in addr;
  int sd;
 
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return -1;
  }
 
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  if(host != NULL) {
    addr.sin_addr.s_addr = inet_addr(host);
  }
 
  if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    return -1;
  }
 
  if(listen(sd, QUEUENUM) < 0) {
    return -1;
  }

  set_signal_handler(cleanup_connection, &sd);

  return sd;
}

static void *connection_co_worker(void *data) {
  struct conninfo *cinfo = (struct conninfo*)data;
  void *ret = NULL;

  if(cinfo->handler != NULL) {
    ret = cinfo->handler(cinfo);
  }

  close(cinfo->sock);
  free(cinfo);

  return ret;
}

static void *do_connection_worker(char *host, int port, void *(*handler)(struct conninfo *)) {
  int sd;

  sd = init_connection(host, port);
  if(sd < 0) {
    err("[connection_worker] failed to create and initialize a connection");
    return NULL;
  }

  info("NewtMQ is ready to accept requests (port: %d)", port);
 
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
      cinfo->handler = handler;

      if(pthread_create(&thread_id, NULL, connection_co_worker, cinfo)) {
        err("[connection_worker] failed to create connection_co_worker");
      }
    }
  }
 
  return NULL;
}

void *ctrl_connection_worker(void *data) {
  newt_config *conf = (newt_config *)data;

  assert(conf != NULL);

  do_connection_worker(conf->server, conf->ctrl_port, &newtctl_worker);
}

void *connection_worker(void *data) {
  newt_config *conf = (newt_config *)data;

  assert(conf != NULL);

  do_connection_worker(conf->server, conf->port, &stomp_conn_worker);
}

int send_msg(int sock, char *msg) {
  int ret;

  if(msg != NULL) {
    debug("[send_msg] %s [%d]", msg, strlen(msg));
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
