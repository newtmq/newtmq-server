#ifndef __STOMP_MANAGEMENT_H__
#define __STOMP_MANAGEMENT_H__

#include <kazusa/stomp.h>

void *stomp_management_worker(void *data);
int iterate_header(struct list_head *, stomp_header_handler_t *, void *);

#endif
