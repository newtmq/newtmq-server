#ifndef __TEST_CLIENT_H__
#define __TEST_CLIENT_H__

int connect_server(void);
int connect_ctrl_server(void);
int stomp_connect(int);
int stomp_send(int, char *, int);

#endif
