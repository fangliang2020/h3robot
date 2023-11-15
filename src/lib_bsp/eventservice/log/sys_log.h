#ifndef SYS_LOG_H
#define SYS_LOG_H

#ifdef USE_SYS_LOG

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __FILENAME__
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)
#ifndef LOG_TAG
#define LOG_TAG __FILENAME__
#endif
#endif
// ---------------------------------------------------------------------

int sys_log_init();
void __sys_log(const char *mid, const int priority,
               const char *file, const char *func, uint32_t line,
               const char *fmt, ...);
void set_syslog_level(int level);
int get_syslog_level();
bool is_log_to_std();

#ifndef SYS_LOG
#define SYS_LOG(priority, func, line, fmt, arg...)                             \
  do {                                                                         \
    __sys_log(priority, func, line, fmt, ##arg);                               \
  } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif  /* #ifdef USE_SYS_LOG */

#endif /* SYS_LOG_H */
