#include <kazusa/list.h>
#include <kazusa/stomp.h>
#include <kazusa/common.h>

#include <string.h>

#define STATUS_VERSION (1 << 0)

static int handler_version(char *context, void *data) {
  int ret = RET_ERROR;
  int *status = (int *)data;
  
  if(status != NULL) {
    if(strcmp(context, "1.2") == 0) {
      *status |= STATUS_VERSION;
      ret = RET_SUCCESS;
    }
  }

  return ret;
}

stomp_header_handler_t handlers[] = {
  {"accept-version:", handler_version},
  {0},
};

static int send_connected_msg(int sock) {
  char *msg[] = {
    "CONNECTED\n",
    "version:1.2\n",
    NULL,
  };

  return RET_SUCCESS;
}

frame_t *handler_stomp_connect(frame_t *frame) {
  int header_status = 0;

  if(iterate_header(&frame->h_attrs, handlers, &header_status) == RET_ERROR) {
    stomp_send_error(frame->sock, "failed to validate header\n");
  }

  if((header_status & STATUS_VERSION) == 0) {
    stomp_send_error(frame->sock, "version negotiation failure\n");
  }

  send_connected_msg(frame->sock);

  return NULL;
}
