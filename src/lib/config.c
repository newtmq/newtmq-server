#include <kazusa/config.h>
#include <kazusa/common.h>
#include <kazusa/logger.h>

static cfg_opt_t config_opts[] = {
  CFG_STR("server", "localhost", CFGF_NONE),
  CFG_INT("port", 61613, CFGF_NONE),
  CFG_STR("loglevel", NULL, CFGF_NONE),
  CFG_STR("logfile", "/tmp/kazusad.log", CFGF_NONE),
  CFG_BOOL("debug", cfg_false, CFGF_NONE),
  CFG_END()
};

int load_config(char *confpath, kd_config *config) {
  int ret;
  cfg_t *cfg;

  if(config == NULL) {
    return RET_ERROR;
  }

  // create and initialize cfg_t
  cfg = cfg_init(config_opts, CFGF_NOCASE);

  ret = cfg_parse(cfg, confpath);
  if(ret == CFG_PARSE_ERROR) {
    return RET_ERROR;
  }

  config->server   = cfg_getstr(cfg, "server");
  config->port     = cfg_getint(cfg, "port");
  config->logfile  = cfg_getstr(cfg, "logfile");
  config->debug    = cfg_getbool(cfg, "debug");

  return RET_SUCCESS;
}
