#include <newt/config.h>
#include <test.h>

#include "client.h"
#include "config.h"
#include "connection.h"

int connect_server() {
  newt_config config = {0};
  int sock = -1;

  set_config(&config);
  if(config.port > 0) {
    sock = open_connection(config.port);
  }

  return sock;
}
