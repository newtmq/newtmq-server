#ifndef __PERSISTENT_WORKER_H__
#define __PERSISTENT_WORKER_H__

#include <newt/list.h>
#include <newt/frame.h>
#include <newt/config.h>

struct frame_info {
  frame_t *frame;
  struct list_head list;
};

int initialize_persistent_worker(newt_config *);
//int cleanup_persistent_worker();

void *persistent_worker(void *);

int unpersist();
int persist_frame(frame_t *, char *);
int update_index_sent(const char *, frame_t *);

#endif
