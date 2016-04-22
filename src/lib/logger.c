#include <kazusa/logger.h>
#include <kazusa/common.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#define TIME_BUF_LEN 64

static int curr_level = DEFAULT_LOG_LEVEL;

static void show_loglevel(int level) {
  switch(level) {
    case LOG_DEBUG:
      printf("[DEBUG] ");
      break;
    case LOG_INFO:
      printf("[ INFO] ");
      break;
    case LOG_WARN:
      printf("[ WARN] ");
      break;
    case LOG_ERROR:
      printf("[ERROR] ");
      break;
  }
}

int set_logger(char *level) {
  int ret = RET_SUCCESS;

  if(level == NULL) {
    return RET_ERROR;
  }

  if(strcmp(level, "DEBUG") == 0) {
    curr_level = LOG_DEBUG;
  } else if(strcmp(level, "INFO") == 0) {
    curr_level = LOG_INFO;
  } else if(strcmp(level, "WARN") == 0) {
    curr_level = LOG_WARN;
  } else if(strcmp(level, "ERROR") == 0) {
    curr_level = LOG_ERROR;
  } else {
    ret = RET_ERROR;
  }

  return ret;
}

void logger(int level, char *fmt, ...) {
  va_list list;
  time_t now;
  struct tm *tminfo;
  char timebuf[TIME_BUF_LEN];

  if(level >= curr_level) {
    show_loglevel(level);

    time(&now);
    tminfo = localtime(&now);
    strftime(timebuf, TIME_BUF_LEN, "%Y/%m/%d %H:%M:%S", tminfo);

    printf("%s > ", timebuf);

    va_start(list, fmt);
    vprintf(fmt, list);
    printf("\n");
    va_end(list);
  }
}

static void do_logger(int level, char *fmt, va_list list) {
  time_t now;
  struct tm *tminfo;
  char timebuf[TIME_BUF_LEN];

  if(level >= curr_level) {
    show_loglevel(level);

    time(&now);
    tminfo = localtime(&now);
    strftime(timebuf, TIME_BUF_LEN, "%Y/%m/%d %H:%M:%S", tminfo);

    printf("%s > ", timebuf);

    vprintf(fmt, list);
    printf("\n");
    va_end(list);
  }
}

void debug(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  do_logger(LOG_DEBUG, fmt, args);
  va_end(args);
}
