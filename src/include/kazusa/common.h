#ifndef __COMMON_H__
#define __COMMON_H__

#define RET_SUCCESS 1
#define RET_ERROR 0

#define GET(obj, value) ((obj->status & value) > 0)
#define DEL(obj, value) (obj->status &= ~(value))
#define SET(obj, value) (obj->status |= value)
#define CLR(obj) (obj->status = 0)

char *strtok_single(char *, char const *);

#endif
