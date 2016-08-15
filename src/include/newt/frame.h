#ifndef __FRAME_H__
#define __FRAME_H__

#include <newt/list.h>
#include <pthread.h>

/* These values specify status of making frame */
#define STATUS_BORN         (1 << 0)
#define STATUS_INPUT_NAME   (1 << 1)
#define STATUS_INPUT_HEADER (1 << 2)
#define STATUS_INPUT_BODY   (1 << 3)
#define STATUS_IN_BUCKET    (1 << 4)
#define STATUS_IN_QUEUE     (1 << 5)

#define FNAME_LEN 12
#define FRAME_ID_LEN 32
#define LD_MAX (1024)

typedef struct stomp_conninfo_t stomp_conninfo_t;

/* This describes a Frame attribute */
typedef struct linedata_t {
  char data[LD_MAX];
  struct list_head list;
  int len;
} linedata_t;

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
  struct list_head l_bucket; // for storing frame to the bucket which has parsed frames
  struct list_head l_transaction;
  struct list_head l_persistent; // for storing waiting list that is handled by persistent worker

  // This parameter is used for persistency to know where farme is already read.
  // specify whole frame size (includes command, header, body)
  int size;

  /* To know the connection state */
  stomp_conninfo_t *cinfo;

  /* This parameters are used for transaction processing */
  int (*transaction_callback)(frame_t *);
  void *transaction_data;
};

frame_t *alloc_frame();
void free_frame(frame_t *);

int parse_frame(frame_t *, char *, int, int *);

#endif
