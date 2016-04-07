#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <kazusa/config.h>

#define BUFSIZE 1024
#define QUEUENUM 10

int daemon_start(kd_config);

#endif
