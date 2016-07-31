#ifndef __STOMP_SENDING_WORKER_H__ 
#define __STOMP_SENDING_WORKER_H__ 

int register_reply_worker(int, char *);
int register_unicast_worker(int, char *, char *);
int register_multicast_worker(int, char *, char *);
int initialize_sending_worker();

#endif
