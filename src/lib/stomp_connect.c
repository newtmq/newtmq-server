#include <kazusa/list.h>
#include <kazusa/stomp.h>
#include <kazusa/common.h>

#include <string.h>
#include <assert.h>

#define STATUS_VERSION (1 << 0)

typedef struct authinfo {
  char *userid;
  char *passwd;
} authinfo_t;

static int handler_login(char *context, void *data) {
  authinfo_t *authinfo = (authinfo_t *)data;
  if(authinfo != NULL) {
    authinfo->userid = context;
  }

  return RET_SUCCESS;
}

static int handler_passcode(char *context, void *data) {
  authinfo_t *authinfo = (authinfo_t *)data;
  if(authinfo != NULL) {
    authinfo->passwd = context;
  }

  return RET_SUCCESS;
}

static stomp_header_handler_t handlers[] = {
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
  authinfo_t auth = {0};

  assert(frame != NULL);
  assert(frame->cinfo != NULL);

  if(iterate_header(&frame->h_attrs, handlers, &auth) == RET_ERROR) {
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  CLR(frame->cinfo);
  SET(frame->cinfo, STATE_CONNECTED);

  /* XXX: needs authentication and authorization processing */
  printf("[debug] (handler_stomp_connect) userid: %s\n", auth.userid);
  printf("[debug] (handler_stomp_connect) passwd: %s\n", auth.passwd);

  send_connected_msg(frame->sock);

  return NULL;
}
