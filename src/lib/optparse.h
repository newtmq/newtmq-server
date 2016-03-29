#ifndef __OPTPARSE_H__
#define __OPTPARSE_H__

struct arguments {
  char *config_path;
};

int parse_opt(int, char **, struct arguments *);

#endif
