#ifndef __STOMP_H__
#define __STOMP_H__

#include <kazusa/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define FNAME_LEN 64
#define ATTR_LEN 512

#define STATUS_INIT       (1 << 0)
#define STATUS_PROCESSING (1 << 1)
#define STATUS_IN_BUCKET  (1 << 2)
#define STATUS_IN_QUEUE   (1 << 3)

typedef struct frame_bucket_t {
  pthread_mutex_t mutex;
  struct list_head l_frame_h;
} frame_bucket_t;

/* This describes STOMP Frame*/
typedef struct frame_t {
  char name[FNAME_LEN];
  int sock;
  unsigned int status;
  struct list_head l_attrs_h;
  char *body;
  unsigned int body_len;
  struct list_head l_bucket;
} frame_t;

/* This describes a Frame attribute */
typedef struct frame_attr_t {
  char data[ATTR_LEN];
  struct list_head l_frame;
} frame_attr_t;


int stomp_init_bucket();
int stomp_cleanup();
int stomp_recv_data(char *, int, void *);
void *stomp_init_connection();

extern frame_bucket_t stomp_frame_bucket;

#endif
