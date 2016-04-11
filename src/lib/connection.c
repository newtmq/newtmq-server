#include <kazusa/connection.h>
#include <kazusa/config.h>
#include <kazusa/stomp.h>
#include <kazusa/signal.h>
#include <kazusa/common.h>

struct sock_info {
  int sd;
  int acc_sd;
};

static int cleanup_connection(void *data) {
  struct sock_info *sinfo = (struct sock_info *)data;
  int ret = RET_ERROR;

  if(sinfo == NULL) {
    close(sinfo->acc_sd);
    close(sinfo->sd);

    ret = RET_SUCCESS;
  }

  return ret;
}

int daemon_start(kd_config conf) {
  struct sockaddr_in addr;
  struct sock_info sinfo;
 
  if((sinfo.sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }
 
  addr.sin_family = AF_INET;
  addr.sin_port = htons(conf.port);
  addr.sin_addr.s_addr = INADDR_ANY;
 
  if(bind(sinfo.sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return -1;
  }
 
  if(listen(sinfo.sd, QUEUENUM) < 0) {
    perror("listen");
    return -1;
  }
 
  struct sockaddr_in from_addr;
  socklen_t sin_size = sizeof(struct sockaddr_in);
  if((sinfo.acc_sd = accept(sinfo.sd, (struct sockaddr *)&from_addr, &sin_size)) < 0) {
    perror("accept");
    return -1;
  }

  /* initialize processing after established connection */
  set_signal_handler(cleanup_connection, &sinfo);

  char buf[BUFSIZE];
  void *driver_cache = NULL;
  while(1) {
    if(recv(sinfo.acc_sd, buf, sizeof(buf), 0) < 0) {
      perror("recv");
      break;
    }

    stomp_recv_data(buf, strlen(buf), sinfo.acc_sd, &driver_cache);
  }
 
  return 0;
}

int send_msg(int sock, char **msg) {
  int i;
  char *line;

  for(i=0; (line = msg[i])!=NULL; i++) {
    if(send(sock, line, strlen(line), 0) < 0) {
      printf("[ERROR] failed to send message [%s]\n", line);
      return RET_ERROR;
    }
  }
  send(sock, "\0", 1, 0);

  return RET_SUCCESS;
}
