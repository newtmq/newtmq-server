#ifndef __TEST_CONNECTION_H__
#define __TEST_CONNECTION_H__

#define RECV_TIMEOUT 3

int open_connection(int);
int mysend(int, char *, int, int);

#endif
