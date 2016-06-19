#ifndef __STOMP_H__
#define __STOMP_H__

#include <newt/list.h>
#include <newt/connection.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define FNAME_LEN 12
#define FRAME_ID_LEN 32
#define CONN_ID_LEN 10
#define LD_MAX (1024)

/* These values specify status of making frame */
#define STATUS_BORN         (1 << 0)
#define STATUS_INPUT_NAME   (1 << 1)
#define STATUS_INPUT_HEADER (1 << 2)
#define STATUS_INPUT_BODY   (1 << 3)
#define STATUS_IN_BUCKET    (1 << 4)
#define STATUS_IN_QUEUE     (1 << 5)

/* These values specify state of connection */
#define STATE_INIT (1 << 0)
#define STATE_CONNECTED (1 << 1)

#define IS_BL(buf) (buf != NULL && buf[0] == '\0')
#define IS_NL(buf) (buf != NULL && (buf[0] == '\n' || buf[0] == '\r'))

typedef struct stomp_conninfo_t stomp_conninfo_t;

typedef struct frame_bucket_t {
  pthread_mutex_t mutex;
  struct list_head h_frame;
} frame_bucket_t;

/* This describes STOMP Frame*/
typedef struct frame_t frame_t;
struct frame_t {
  char name[FNAME_LEN];
  char id[FRAME_ID_LEN];
  int sock;
  unsigned int status;
  int contentlen; // content length which is specified in header
  int has_contentlen; // content length which is actually read
  pthread_mutex_t mutex_header;
  pthread_mutex_t mutex_body;
  struct list_head h_attrs;
  struct list_head h_data;
  struct list_head l_bucket;
  struct list_head l_transaction;

  /* To know the connection state */
  stomp_conninfo_t *cinfo;

  /* This parameters are used for transaction processing */
  int (*transaction_callback)(frame_t *);
  void *transaction_data;
};

/* This describes a Frame attribute */
typedef struct linedata_t {
  char data[LD_MAX];
  struct list_head list;
  int len;
} linedata_t;

/* This is alive during connection is active */
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

frame_t *alloc_frame();
void free_frame(frame_t *);

frame_t *get_frame_from_bucket();

/* This is used over each STOMP handlers */
void stomp_send_error(int, char *);
void stomp_send_receipt(int, char *);
void stomp_send_message(int, frame_t *, struct list_head *);
linedata_t *stomp_setdata(char *, int, struct list_head *, pthread_mutex_t *);

#endif
