#ifndef __STOMP_H__
#define __STOMP_H__

#include <newt/list.h>
#include <newt/connection.h>
#include <newt/frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define CONN_ID_LEN 10
#define LD_MAX (1024)

/* These values specify state of connection */
#define STATE_INIT (1 << 0)
#define STATE_CONNECTED (1 << 1)

#define IS_BL(buf) (buf != NULL && buf[0] == '\0')
#define IS_NL(buf) (buf != NULL && (buf[0] == '\n' || buf[0] == '\r'))

typedef struct frame_bucket_t {
  pthread_mutex_t mutex;
  struct list_head h_frame;
} frame_bucket_t;

/* This describes a Frame attribute */
typedef struct linedata_t {
  char data[LD_MAX];
  struct list_head list;
  int len;
} linedata_t;

/* This is alive during connection is active */
typedef struct stomp_conninfo_t stomp_conninfo_t;
struct stomp_conninfo_t {
  char id[CONN_ID_LEN];
  int status;
  frame_t *frame;
  char *prev_data;
  int prev_len;
};

typedef struct stomp_header_handler {
  char *name;
  int (*handler)(char *, void *, linedata_t *);
} stomp_header_handler_t;

int stomp_init();
void *stomp_conn_worker(struct conninfo *);

frame_t *get_frame_from_bucket();

/* This is used over each STOMP handlers */
void stomp_send_error(int, char *);
void stomp_send_receipt(int, char *);
void stomp_send_message(int, frame_t *, struct list_head *);
linedata_t *stomp_setdata(char *, int, struct list_head *, pthread_mutex_t *);

#endif
