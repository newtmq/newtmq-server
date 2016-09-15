#ifndef __TEST_CONNECTION_H__
#define __TEST_CONNECTION_H__

#define RECV_TIMEOUT 3

int open_connection(int);
int test_send(int, char *, int, int);

#endif
