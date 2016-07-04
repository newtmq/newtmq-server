#include <newt/common.h>

void gen_random(char *s, const int len) {
  static const char alphanum[] = 
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  int i;
  
  for (i=0; i<len-1; i++) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }
  
  s[len-1] = 0;
}
