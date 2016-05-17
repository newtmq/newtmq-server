#ifndef __NWETCTL_H__
#define __NWETCTL_H__

#include <newt/connection.h>

#define NEWTCTL_LIST_QUEUES (1 << 0)

#define DATALEN (4096)

#define NEWTCTL_MEMBERS (3)
typedef struct newtctl_t {
  int command;
  int status;
  char context[DATALEN];
} newtctl_t;

void *newtctl_worker(struct conninfo *);

// newt_ctrl_commands
int newtctl_list_queues(newtctl_t *);

#endif
