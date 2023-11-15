#include "sys_log.h"

#ifdef USE_SYS_LOG

#include <map>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include "eventservice/util/time_util.h"
#include "eventservice/util/ml_pipe.h"
#include "eventservice/util/proc_util.h"

#define gettid() syscall(__NR_gettid)

#define	INTERNAL_NOPRI 0x10       /* the "no priority" priority */

static std::map<int, const char*> prioritynames = {
  {LOG_ALERT,       "alert"},
  {LOG_CRIT,        "crit"},
  {LOG_DEBUG,       "debug"},
  {LOG_EMERG,       "emerg"},
  {LOG_ERR,         "err"},
  {LOG_ERR,         "error"},   /* DEPRECATED */
  {LOG_INFO,        "info"},
  {INTERNAL_NOPRI,  "none"},    /* INTERNAL */
  {LOG_NOTICE,      "notice"},
  {LOG_EMERG,       "panic"},   /* DEPRECATED */
  {LOG_WARNING,     "warn"},    /* DEPRECATED */
  {LOG_WARNING,     "warning"},
  {-1,              "none"}
};

/* 非调试模式下，默认log等级为只输出错误信息到syslog */
static int log_level = LOG_ERR;
static pthread_t log_ctrl_thread = -1;
static const char *g_fifo_path = "/tmp/log/";
static std::string app_fifo;

void *log_ctrl_thread_cb(void *context) {
  char buf[64] = {0};
  char arg_name[64] = {0};
  int arg_val = 0;
  int ret = 0;
  const char *progname = chml::XProcUtil::get_prog_short_name();
  chml::MLPipe ml_pipe("/tmp/log", progname);

  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(0, &cpu_set);
  ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
  if (ret < 0) {
    printf("pthread_setaffinity_np failed\n");
  }

  prctl(PR_SET_NAME, "log_ctrl");

  while (true) {
    memset(buf, 0, sizeof(buf));
    memset(arg_name, 0, sizeof(arg_name));
    ml_pipe.Read(buf, sizeof(buf));
    sscanf(buf, "%s %d", arg_name, &arg_val);
    if (strcmp(arg_name, "loglevel") == 0) {
      log_level = arg_val;
      printf("set app: %s level to %d\n", progname, log_level);
    }
  }

  return NULL;
}

int sys_log_init() {
  int ret = -1;

  openlog(NULL, /* LOG_CONS | */ LOG_NDELAY | LOG_PERROR, LOG_USER);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 128 * 1024);
  ret = pthread_create(&log_ctrl_thread, NULL, log_ctrl_thread_cb, NULL);
  if (ret < 0) {
    printf("create log ctrl thread failed\n");
    return -1;
  }
  pthread_detach(log_ctrl_thread);

  return 0;
}

void set_syslog_level(int level) {
  log_level = level;
}

int get_syslog_level() {
  return log_level;
}

void __sys_log(const char *mid, const int priority,
               const char *file, const char *func, uint32_t line,
               const char *fmt, ...) {
  if (priority > log_level)
    return;

  const int buf_len = 1024;
  char buffer[buf_len] = {0};
  va_list args;

  va_start(args, fmt);
  vsnprintf(buffer, buf_len-1, fmt, args);
  va_end(args);

  char time_str[64] = {0};
  chml::XTimeUtil::TimeMSLocalToString(time_str, sizeof(time_str));
  if (mid != NULL) {
    syslog(LOG_USER | priority, "%s [%d] [%s] <%s> %s:%d %s, %s", mid, gettid(), time_str, prioritynames[priority], file, line, func, buffer);
  } else {
    syslog(LOG_USER | priority, "[%d] [%s] <%s> %s:%d %s, %s", gettid(), time_str, prioritynames[priority], file, line, func, buffer);
  }
}

#endif  /* #ifndef USE_SYS_LOG */
