
#include "proc_util.h"
#include "string_util.h"
#include "eventservice/base/basictypes.h"
#include <errno.h>
#ifndef WIN32
#include <sys/syscall.h>    // SYS_gettid
#endif

# ifdef __USE_GNU
#if !defined(PROJECT_H1S)
extern char *__progname;
extern char *program_invocation_name;
extern char *program_invocation_short_name;
#endif
#endif

namespace chml {

bool XProcUtil::check_process_running(const char *process_name) {
  char ps[128];
  char buff[512];
  FILE *ptr = NULL;
#ifndef WIN32
  strcpy(buff, "ABNORMAL");
  sprintf(ps, "ps -e | grep -c ' %s</p>'", process_name);
  if ((ptr = popen(ps, "r")) != NULL) {
    while (fgets(buff, 512, ptr) != NULL) {
      if (atoi(buff) >= 2) {
        pclose(ptr);
        return true;
      }
    }
  }
  if (strcmp(buff, "ABNORMAL") == 0) /*ps command error*/
    return false;

  if (ptr != NULL) {
    pclose(ptr);
  }
#endif
  return false;
}

int XProcUtil::self_pid() {
  int pid = -1;
#ifdef WIN32
  pid = GetCurrentProcessId();
#else
  pid = (int)getpid();
#endif
  return pid;
}

int XProcUtil::self_tid() {
  int tid = -1;
#ifdef WIN32
  tid = GetCurrentThreadId();
#else
  tid = syscall(SYS_gettid);
#endif
  return tid;
}

const char *XProcUtil::get_prog_name() {
# ifdef __USE_GNU
  return program_invocation_name;
#else
  static char buf[128] = {0};
  memset(buf, sizeof(buf), 0);
  FILE *fp = fopen("/proc/self/cmdline", "r");
  if (fp) {
    fread(buf, sizeof(buf), 1, fp);
    fclose(fp);
  }
  return buf;
#endif
}

const char *XProcUtil::get_prog_short_name() {
# ifdef __USE_GNU
  return program_invocation_short_name;
#else
  static char buf[128] = {0};
  memset(buf, sizeof(buf), 0);
  FILE *fp = fopen("/proc/self/cmdline", "r");
  if (fp) {
    fread(buf, sizeof(buf), 1, fp);
    fclose(fp);
  }
  return buf;
#endif
}

int XProcUtil::set_affinity(const std::set<int> &cpus) {
#ifndef WIN32
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  for (auto cpu : cpus) {
    CPU_SET(cpu, &cpu_set);
  }
  sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpu_set);
#endif
  return 0;
}

bool XProcUtil::set_sched_priority(int priority) {
#ifndef WIN32
  pid_t pid = getpid();
  struct sched_param param;
  param.sched_priority = priority;
  if (sched_setscheduler(pid, SCHED_RR, &param) == -1) {
    printf("!!! sched_setscheduler error. pid = %d, pname = %s !!!\n", pid, get_prog_name());
    return false;
  }
  printf("sched_getscheduler = %d", sched_getscheduler(pid));
#endif
  return true;
}

bool XProcUtil::set_sched_priority_max() {
#ifndef WIN32
  return set_sched_priority(sched_get_priority_max(SCHED_RR));
#else
  return true;
#endif
}

#ifndef WIN32
/**
 *  \brief  执行命令并取得命令输出字符串.
 *  \param  cmd 命令字符串.
 *  \param  result 命令输出.
 *  \param  timeout 命令超时时间(seconds,默认为5秒).
 *  \return 命令执行成功返回STATUS_OK,命令执行失败返回STATUS_ERROR.
 *  \note   注意:必须确保cmd能够自动结束,否则该函数将一直等待,无法退出!
 */
int XProcUtil::execute(std::string cmd, std::string& result, int timeout) {
  FILE* fp = popen(cmd.c_str(), "r");
  if (NULL == fp) {
    printf("ProcUtil::execute(%s), popen failed.\n", cmd.c_str());
    return -1;
  }
  //TRACE_REL("command:[%s]\n",cmd.c_str());
  result = "";
  if (timeout <= 0) {
    timeout = 500;
  }
  int tout = timeout;
  time_t startTime = ::time(NULL);
  while (1) {
    time_t endTime = ::time(NULL);
    if ((endTime - startTime) < 0 ||
        (endTime - startTime) >= timeout) {
      break;
    }
    if (tout <= 0) {
      break;
    }
    int fd = fileno(fp);
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int ret = select(fd + 1, &readfds, NULL, NULL, &tv);
    if (ret <= 0 || (!FD_ISSET(fd, &readfds))) {
      tout -= 1;
      continue;
    } else {
      char buf[256] = { 0 };
      ret = fread(buf, 1, sizeof(buf), fp);
      if (ret <= 0) {
        break;
      }
      result += std::string(buf, ret);
    }
  }
  pclose(fp);
  return 0;
}

/**
 *  \brief  执行命令并取得命令输出字符串.
 *  \param  cmd 命令字符串.
 *  \param  timeout 命令超时时间(seconds,默认为5秒).
 *  \return 成功返回命令输出,失败返回空字符串.
 *  \note   注意:必须确保cmd能够自动结束,否则该函数将一直等待,无法退出!
 */
std::string XProcUtil::execute(std::string cmd, int timeout) {
  std::string result = "";
  FILE* fp = popen(cmd.c_str(), "r");
  if (NULL == fp) {
    printf("ProcUtil::execute(%s), popen failed.\n", cmd.c_str());
    return result;
  }
  //TRACE_REL("command:[%s]\n",cmd.c_str());
  if (timeout <= 0) {
    timeout = 500;
  }
  int tout = timeout;
  time_t startTime = ::time(NULL);
  while (1) {
    time_t endTime = ::time(NULL);
    if ((endTime - startTime) < 0 ||
        (endTime - startTime) >= timeout) {
      break;
    }
    if (tout <= 0) {
      break;
    }
    int fd = fileno(fp);
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int ret = select(fd + 1, &readfds, NULL, NULL, &tv);
    if (ret <= 0 || (!FD_ISSET(fd, &readfds))) {
      tout -= 1;
      continue;
    } else {
      char buf[256] = { 0 };
      ret = fread(buf, 1, sizeof(buf), fp);
      if (ret <= 0) {
        break;
      }
      result += std::string(buf, ret);
    }
  }
  pclose(fp);
  return result;
}

//std::vector<int> XProcUtil::getPidsByName(const char* processName) {
//  std::vector<int> pidList;
//  Directory dir;
//  dir.enter("/proc");
//  std::vector<std::string> fileNameList = dir.listAll();
//  int num = fileNameList.size();
//  for (int i = 0; i < num; i++) {
//    if (!XStrUtil::is_int(fileNameList[i].c_str())) {
//      continue;
//    }
//    File file;
//    std::string statusFile = "/proc/" + fileNameList[i] + "/status";
//    if (!file.open(statusFile.c_str(), IO_MODE_RD_ONLY)) {
//      printf("ProcUtil::getPidsByName,can't open file:%s.\n", statusFile.c_str());
//      continue;
//    }
//    std::string firstLine;
//    if (STATUS_ERROR == file.readLine(firstLine)) {
//      printf("ProcUtil::getPidsByName,read file error:%s.\n", statusFile.c_str());
//      continue;
//    }
//    std::vector<std::string> result = StrUtil::splitString(firstLine, ":");
//    if (result.size() != 2) {
//      continue;
//    }
//    std::string pid = StrUtil::trimEndingBlank(result[1]);
//    if (pid == processName) {
//      pidList.push_back(atoi(fileNameList[i].c_str()));
//    }
//  }
//  return pidList;
//}
#endif
}  // namespace chml
