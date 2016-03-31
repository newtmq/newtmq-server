#ifndef __OPTPARSE_H__
#define __OPTPARSE_H__

struct cmd_args {
  char *config_path;
};

int parse_opt(int, char **, struct cmd_args *);

#endif
