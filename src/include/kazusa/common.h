#ifndef __COMMON_H__
#define __COMMON_H__

#define RET_SUCCESS 1
#define RET_ERROR 0

#define GET_STATUS(obj, value) ((obj->status & value) > 0)
#define DEL_STATUS(obj, value) (obj->status &= ~(value))
#define SET_STATUS(obj, value) (obj->status |= value)
#define CLR_STATUS(obj) (obj->status = 0)

#endif
