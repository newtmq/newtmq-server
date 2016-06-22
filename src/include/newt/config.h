#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <confuse.h>

typedef struct newt_config {
  char *server;
  int port;
  int ctrl_port;
  char *logfile;
  cfg_bool_t debug;
  char *loglevel;
} newt_config;

int load_config(char *, newt_config *);

#endif
