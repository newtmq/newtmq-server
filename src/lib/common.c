#include <stdio.h>
#include <string.h>
#include <kazusa/common.h>

char *strtok_single(char * str, char const * delims) {
  static char  * src = NULL;
  char  *  p,  * ret = 0;

  if(str != NULL){
    src = str;
  }

  if(src == NULL){
    return NULL;
  }

  if((p = strpbrk (src, delims)) != NULL){
    *p  = 0;
    ret = src;
    src = ++p;
  }else if (*src){
    ret = src;
    src = NULL;
  }

  return ret;
}
