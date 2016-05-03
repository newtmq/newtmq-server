#ifndef __LOGGER_H__
#define __LOGGER_H__

#define LOG_DEBUG 1
#define LOG_INFO  2
#define LOG_WARN  3
#define LOG_ERROR 4

#define DEFAULT_LOG_LEVEL LOG_INFO

void debug(char *, ...);
void info(char *, ...);
void warn(char *, ...);
void err(char *, ...);

int set_logger(char *);

#endif
