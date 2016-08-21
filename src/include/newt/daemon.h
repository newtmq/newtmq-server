#ifndef __DAEMON_H__
#define __DAEMON_H__

#include <newt/config.h>

int daemon_initialize(newt_config *);
int daemon_start(newt_config *);

#endif
