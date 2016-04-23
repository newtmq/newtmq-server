#include <kazusa/connection.h>
#include <kazusa/config.h>
#include <kazusa/stomp.h>
#include <kazusa/signal.h>
#include <kazusa/common.h>
#include <kazusa/logger.h>

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

static int cleanup_co_worker(void *data) {
  struct conninfo *cinfo = (struct conninfo *)data;
  int ret = RET_ERROR;

  if(cinfo != NULL) {
    close(cinfo->sock);

    if(cinfo->protocol_data != NULL) {
      free(cinfo->protocol_data);
    }

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
  struct conninfo cinfo = {0};
  sighandle_t *handler;
  char buf[RECV_BUFSIZE];

  if(data == NULL) {
    return NULL;
  }

  pthread_mutex_init(&cinfo.mutex, NULL);
  INIT_LIST_HEAD(&cinfo.h_buf);
  cinfo.sock = *(int *)data;
  cinfo.protocol_data = (void *)stomp_conn_init();

  /* initialize processing after established connection */
  handler = set_signal_handler(cleanup_co_worker, &cinfo);

  while(1) {
    int len;

    memset(buf, 0, RECV_BUFSIZE);
    len = recv(cinfo.sock, buf, sizeof(buf), 0);
    if(! len) {
      break;
    }

    stomp_recv_data(buf, len, cinfo.sock, cinfo.protocol_data);
  }

  /* cancel to parse of the current frame */
  stomp_conn_finish(cinfo.protocol_data);

  del_signal_handler(handler);

  close(cinfo.sock);

  return NULL;
}

void *connection_worker(void *data) {
  kd_config *conf = (kd_config *)data;
  struct sockaddr_in addr;
  int sd;
  int acc_sd;
 
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return NULL;
  }
 
  addr.sin_family = AF_INET;
  addr.sin_port = htons(conf->port);
  addr.sin_addr.s_addr = INADDR_ANY;
 
  if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return NULL;
  }
 
  if(listen(sd, QUEUENUM) < 0) {
    perror("listen");
    return NULL;
  }

  set_signal_handler(cleanup_connection, &sd);
 
  struct sockaddr_in from_addr;
  socklen_t sin_size = sizeof(struct sockaddr_in);
  pthread_t thread_id;
  while(1) {
    if((acc_sd = accept(sd, (struct sockaddr *)&from_addr, &sin_size)) < 0) {
      perror("accept");
      return NULL;
    }

    pthread_create(&thread_id, NULL, &connection_co_worker, &acc_sd);
  }

  info("(connection_worker) finished");
 
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
