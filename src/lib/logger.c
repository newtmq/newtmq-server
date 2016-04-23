#include <kazusa/logger.h>
#include <kazusa/common.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TIME_BUF_LEN 64

static int curr_level = DEFAULT_LOG_LEVEL;
static pthread_mutex_t log_mutex;

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

  pthread_mutex_init(&log_mutex, NULL);

  return ret;
}

static void do_logger(int level, char *fmt, va_list list) {
  time_t now;
  struct tm *tminfo;
  char timebuf[TIME_BUF_LEN];

  pthread_mutex_lock(&log_mutex);
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
  pthread_mutex_unlock(&log_mutex);
}

void debug(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  do_logger(LOG_DEBUG, fmt, args);
  va_end(args);
}
void info(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  do_logger(LOG_INFO, fmt, args);
  va_end(args);
}
void warn(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  do_logger(LOG_WARN, fmt, args);
  va_end(args);
}
void err(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  do_logger(LOG_ERROR, fmt, args);
  va_end(args);
}
