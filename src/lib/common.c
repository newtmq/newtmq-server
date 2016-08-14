#include <newt/common.h>
#include <sys/stat.h>
#include <stdio.h>

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

unsigned long get_hash(unsigned char *str) {
  unsigned long hash = 5381;
  int c;

  while (c = *str++) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }

  return hash;
}

void mkdir_recursive(char *dir) {
  char tmp[512] = {0};
  char *p = NULL;
  int len;
  mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

  len = snprintf(tmp, sizeof(tmp),"%s",dir);
  if(tmp[len - 1] == '/') {
    tmp[len - 1] = 0;
  }
  for(p = tmp + 1; *p; p++) {
    if(*p == '/') {
      *p = 0;
      mkdir(tmp, mode);
      *p = '/';
    }
  }
  mkdir(tmp, mode);
}
