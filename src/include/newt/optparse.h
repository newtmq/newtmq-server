#ifndef __OPTPARSE_H__
#define __OPTPARSE_H__

#include <argp.h>

struct cmd_args {
  char *config_path;
};

int parse_opt(int, char **, struct cmd_args *);

#endif
