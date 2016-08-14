#ifndef __PERSISTENT_WORKER_H__
#define __PERSISTENT_WORKER_H__

#include <newt/list.h>
#include <newt/frame.h>
#include <newt/config.h>

int initialize_persistent_worker(newt_config *);
int cleanup_persistent_worker();
int load_from_persistent(char *);

int persistent(const char *, frame_t *);
int update_index_sent(const char *, frame_t *);

#endif
