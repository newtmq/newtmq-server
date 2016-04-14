#include <kazusa/list.h>
#include <kazusa/stomp.h>
#include <kazusa/common.h>

#include <string.h>

#define STATUS_VERSION (1 << 0)

typedef struct conninfo {
  char *userid;
  char *passwd;
} conninfo_t;

static int handler_login(char *context, void *data) {
  conninfo_t *conninfo = (conninfo_t *)data;
  if(conninfo != NULL) {
    conninfo->userid = context;
  }

  return RET_SUCCESS;
}

static int handler_passcode(char *context, void *data) {
  conninfo_t *conninfo = (conninfo_t *)data;
  if(conninfo != NULL) {
    conninfo->passwd = context;
  }

  return RET_SUCCESS;
}

stomp_header_handler_t handlers[] = {
  {"login:", handler_login},
  {"passcode:", handler_passcode},
  {0},
};

static int send_connected_msg(int sock) {
  char *msg[] = {
    "CONNECTED\n",
    "version:1.2\n",
    "\n",
    "\0",
    NULL,
  };

  send_msg(sock, msg);

  return RET_SUCCESS;
}

frame_t *handler_stomp_connect(frame_t *frame) {
  conninfo_t cinfo = {0};

  if(iterate_header(&frame->h_attrs, handlers, &cinfo) == RET_ERROR) {
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  /* XXX: needs authentication and authorization processing */
  printf("[debug] (handler_stomp_connect) userid: %s\n", cinfo.userid);
  printf("[debug] (handler_stomp_connect) passwd: %s\n", cinfo.passwd);

  send_connected_msg(frame->sock);

  return NULL;
}
