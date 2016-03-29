#include <argp.h>
#include "optparse.h"

static struct argp_option options[] = {
  {"config", 'c', "config_path", 0, "filepath of configuration for kazusad"},
  {0}
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {
  return 0;
}

static struct argp argp = {options, _parse_opt, NULL, NULL};

int parse_opt(int argc, char **argv, struct arguments *args) {
  argp_parse(&argp, argc, argv, 0, 0, &args);

  return 0;
}
