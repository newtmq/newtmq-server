#include <kazusa/connection.h>
#include <kazusa/config.h>
#include <kazusa/stomp.h>
#include <signal.h>
#include <pthread.h>

static pthread_t thread_id;

void int_handler(int code) {
  stomp_cleanup();

  pthread_cancel(thread_id);
}

void *receive_worker(void *data) {
  void *driver_cache = NULL;
  char buf[BUFSIZE];

  if(data != NULL) {
    int sock = *(int *)data;

    while(1) {
      if(recv(sock, buf, sizeof(buf), 0) < 0) {
        perror("recv");
        break;
      }
  
      stomp_recv_data(buf, strlen(buf), sock, &driver_cache);
    }
  }

  return NULL;
}

int daemon_start(kd_config conf) {
  struct sockaddr_in addr;
  int sd, acc_sd;
 
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }
 
  addr.sin_family = AF_INET;
  addr.sin_port = htons(conf.port);
  addr.sin_addr.s_addr = INADDR_ANY;
 
  if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return -1;
  }
 
  if(listen(sd, QUEUENUM) < 0) {
    perror("listen");
    return -1;
  }
 
  struct sockaddr_in from_addr;
  socklen_t sin_size = sizeof(struct sockaddr_in);
  if((acc_sd = accept(sd, (struct sockaddr *)&from_addr, &sin_size)) < 0) {
    perror("accept");
    return -1;
  }

  signal(SIGINT, int_handler);

  pthread_create(&thread_id, NULL, &receive_worker, &acc_sd);

  pthread_join(thread_id, NULL);
 
  close(acc_sd);
  close(sd);
 
  return 0;
}
