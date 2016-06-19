#ifndef __STOMP_WORKER_SENDING_H__
#define __STOMP_WORKER_SENDING_H__

int register_stomp_sending_worker(int sock, char *destination);
int initialize_worker_sending();

#endif
