#include <test.h>

#include <newt/config.h>
#include <pthread.h>

static pthread_mutex_t mutex_config;

int set_config(newt_config *config) {
  char confpath[512];

  getcwd(confpath, sizeof(confpath));
  sprintf(confpath, "%s/%s", confpath, TEST_CONFIG);

  // because load_config is not thread-safe
  pthread_mutex_lock(&mutex_config);
  {
    load_config(confpath, config);
  }
  pthread_mutex_unlock(&mutex_config);
}

void init_config() {
  pthread_mutex_init(&mutex_config, NULL);
}
