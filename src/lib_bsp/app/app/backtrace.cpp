#if defined(LITEOS) || defined(WIN32)
void backtrace_init() {
  return;
}
#else

#include <dlfcn.h>
#include <stdlib.h>
#include <unwind.h>
#include <assert.h>
#include <stdio.h>
//#include <link.h>         // required for __ELF_NATIVE_CLASS
#include <bits/wordsize.h>  // required for (__ELF_NATIVE_CLASS)__WORDSIZE
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fstream>

#include "eventservice/event/thread.h"
#include "eventservice/log/log_client.h"
#include "eventservice/util/proc_util.h"

#if __WORDSIZE == 32
# define WORD_WIDTH 8
#else
/* We assyme 64bits.  */
# define WORD_WIDTH 16
#endif

#define BACKTRACE_DUMPTO_FILE true
#define BACKTRACE_FILE_TOT 7
//const char *bt_file_format = "/tmp/back_tace/out%d";

#if defined(UBUNTU64)
const char *bt_file_format =  "/tmp/bt_file_%d.txt";
#elif defined(LITEOS)
const char *bt_file_format = "bt_file_%d.txt";
#else
const char *bt_file_format = "/userdata/log/system_server_files/bt_file%d.txt";
const char *wdg_file = "/userdata/log/system_server_files/wdg.txt";                                                                                                                                                                                                                                
#endif
static bool g_signal_happened = false;
const char *upload_new = "NEW";
const char *upload_old = "OLD";
struct trace_arg {
  void  **array;
  int     cnt;
  int     size;
};

static const char *signal_to_str(int sig)
{
  switch (sig)
  {
  case SIGHUP:
    return "SIGHUP";
  case SIGINT:
    return "SIGINT";
  case SIGQUIT:
    return "SIGQUIT";
  case SIGILL:
    return "SIGILL";
  case SIGABRT:
    return "SIGABRT";
  case SIGFPE:
    return "SIGFPE";
  case SIGKILL:
    return "SIGKILL";
  case SIGSEGV:
    return "SIGSEGV";
  case SIGPIPE:
    return "SIGPIPE";
  case SIGALRM:
    return "SIGALRM";
  case SIGTERM:
    return "SIGTERM";
  case SIGUSR1:
    return "SIGUSR1";
  case SIGUSR2:
    return "SIGUSR2";
  case SIGCHLD:
    return "SIGCHLD";
  case SIGCONT:
    return "SIGCONT";
  case SIGSTOP:
    return "SIGSTOP";
  case SIGTSTP:
    return "SIGTSTP";
  case SIGTTIN:
    return "SIGTTIN";
  case SIGTTOU:
    return "SIGTTOU";
  default:
    return "SIGDEFAULT";
  }
}

static _Unwind_Reason_Code backtrace_helper(struct _Unwind_Context *ctx, void *a) {
  struct trace_arg *arg = (struct trace_arg *)a;

  // We are first called with address in the __backtrace function. Skip it.
  if (arg->cnt != -1) {
    arg->array[arg->cnt] = (void *)_Unwind_GetIP(ctx);
  }
  if (++arg->cnt == arg->size) {
    return _URC_END_OF_STACK;
  }
  return _URC_NO_REASON;
}

char **backtrace_symbols(void *const *array, int size) {
  Dl_info *info = new Dl_info[size];
  int *status = new int[size];
  int cnt;
  size_t total = 0;
  char **result;

  // Fill in the information we can get from `dladdr'.
  for (cnt = 0; cnt < size; ++cnt) {
    status[cnt] = dladdr(array[cnt], &info[cnt]);
    if (status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0') {
      // We have some info, compute the length of the string which will be
      // "<file-name>(<sym-name>) [+offset].
      total += (strlen(info[cnt].dli_fname ? : "")
                + (info[cnt].dli_sname ? strlen(info[cnt].dli_sname) + 3 + WORD_WIDTH + 3 : 1)
                + WORD_WIDTH + 5);
    } else {
      total += 5 + WORD_WIDTH;
    }
  }

  // Allocate memory for the result.
  result = (char **)malloc(size * sizeof(char *) + total);
  if (result != NULL) {
    char *last = (char *)(result + size);
    for (cnt = 0; cnt < size; ++cnt) {
      result[cnt] = last;
      if (status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0') {
        char buf[20];
        if (array[cnt] >= (void *)info[cnt].dli_saddr) {
          sprintf(buf, "+%#lx",
                  (unsigned long)((unsigned long)array[cnt] - (unsigned long)info[cnt].dli_saddr));
        } else {
          sprintf(buf, "-%#lx",
                  (unsigned long)((unsigned long)info[cnt].dli_saddr - (unsigned long)array[cnt]));
        }

        last += 1 + sprintf(last, "%s%s%s%s%s[%p]",
                            info[cnt].dli_fname ? : "",
                            info[cnt].dli_sname ? "(" : "",
                            info[cnt].dli_sname ? : "",
                            info[cnt].dli_sname ? buf : "",
                            info[cnt].dli_sname ? ") " : " ",
                            array[cnt]);
      } else {
        last += 1 + sprintf(last, "[%p]", array[cnt]);
      }
    }
    assert(last <= (char *)result + size * sizeof(char *) + total);
  }
  if (info) {
    delete[] info;
  }
  if (status) {
    delete[] status;
  }
  return result;
}

// Perform stack unwinding by using the _Unwind_Backtrace.
int backtrace(void **array, int size) {
  struct trace_arg arg;
  arg.array = array;
  arg.size = size;
  arg.cnt = -1;
  if (size >= 1) {
    _Unwind_Backtrace(backtrace_helper, &arg);
  }
  return arg.cnt != -1 ? arg.cnt : 0;
}

int backtrace_file_dump(int num, int nptrs, char **strings, char *thread_name) {
  char filename[100];
  FILE *fp = NULL;
  int idx = BACKTRACE_FILE_TOT - 1;
  time_t cur_t;

  for (int i = 0; i < BACKTRACE_FILE_TOT; i++) {
    sprintf(filename, bt_file_format, i);
    struct stat st;
    int err = stat(filename, &st);
    if (err) {
      idx = i;
      break;
    }
    if (i == 0) {
      idx = i;
      cur_t = st.st_mtim.tv_sec;
    } else {
      if (cur_t > st.st_mtim.tv_sec) {
        idx = i;
        cur_t = st.st_mtim.tv_sec;
      }
    }
  }

  sprintf(filename, bt_file_format, idx);
  fp = fopen(filename, "w");
  if (fp == NULL) {
    return -1;
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);
  time_t secs = tv.tv_sec;
  struct tm *tp = localtime(&secs);

  fprintf(fp, "{\n");
  fprintf(fp, "  \"is_new\": true,\n");
  fprintf(fp, "  \"save_log\": true,\n");
  fprintf(fp, "  \"crash_signal\": \"%s\",\n", signal_to_str(num));
  fprintf(fp, "  \"process_name\": \"%s\",\n", chml::XProcUtil::get_prog_short_name());
  fprintf(fp, "  \"thread_id\": \"%ld\",\n", chml::XProcUtil::self_tid());
  fprintf(fp, "  \"thread_name\": \"%s\",\n", thread_name);
  fprintf(fp, "  \"system_time_local\": \"%u-%u-%u %u:%u:%u\",\n",
          tp->tm_year+1900u, tp->tm_mon + 1U, tp->tm_mday,
          tp->tm_hour, tp->tm_min, tp->tm_sec);

  // The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
  // would produce similar output to the following.
  if (strings == NULL) {
    perror("backtrace_symbols");
    fprintf(fp, "  \"backtrace\": []\n");
    fprintf(fp, "}\n");
    exit(EXIT_FAILURE);
  }
  fprintf(fp, "  \"backtrace\": [\n");
  for (int i = 0; i < nptrs; i++) {
    if (i == nptrs - 1) {
      fprintf(fp, "        \"%s\",\n", strings[i]);
    } else {
      fprintf(fp, "        \"%s\"\n", strings[i]);
    }
  }
  fprintf(fp, "    ]\n");
  fprintf(fp, "}\n");
  fclose(fp);
  return  0;
}

void signal_action(int num) {
#define SIZE 100
  void *buffer[SIZE];
  char **strings;
  char thread_name[100] = {0};
  const char* signame = signal_to_str(num);

  if((SIGTERM == num) || (true == g_signal_happened))
  {
    return ;
  }

  prctl(PR_GET_NAME, thread_name);
#ifndef __FACE__
  LOG_TRACE(LT_SYS, "%04d System signal \"%s\" received, thread:%ld, name:%s",
            LOG_ID_CRASHED_SIGSEGV, signame, chml::XProcUtil::self_tid(), thread_name);
#endif
  Log_FlushCache();
  chml::Thread::SleepMs(2 * 1000);

  printf("\n\nNEW============================================\n"
         "System signal \"%s\" received\nproc: %ld, name: %s, thread: %ld, name:%s\n",
         signame,
         chml::XProcUtil::self_pid(), chml::XProcUtil::get_prog_short_name(),
         chml::XProcUtil::self_tid(), thread_name);
  int nptrs = backtrace(buffer, SIZE);
  printf("backtrace() returned %d addresses\n", nptrs);

  // The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
  // would produce similar output to the following.
  strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < nptrs; i++) {
    printf("%s\n", strings[i]);
  }
  if (BACKTRACE_DUMPTO_FILE) {
    backtrace_file_dump(num, nptrs, strings, thread_name);
  }
  fflush(stdout);
  free(strings);
  g_signal_happened = true;
}

void backtrace_init() {
  {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);
  }

  {
    struct sigaction act, oldact;
    act.sa_handler = signal_action;
    act.sa_flags = SA_NODEFER | SA_RESETHAND;
    sigaction(SIGSEGV, &act, &oldact);
    // sigaction(SIGILL, &act, &oldact);
    sigaction(SIGBUS, &act, &oldact);
    sigaction(SIGABRT, &act, &oldact);
    sigaction(SIGTERM, &act, &oldact);
  }
}

bool backtrace_get_log(char *str, size_t len) {
  char filename[256];
  char bt_info[2048];
  std::string bt_info_str;
  size_t bt_info_len;
  Json::Value bt_info_jv;

  for (int i = 0; i < BACKTRACE_FILE_TOT; i++) {
    sprintf(filename, bt_file_format, i);
    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) {
      fprintf(stderr, "open %s failed.\n", filename);
      continue;
    }
    file.seekg(0, std::ios::end);
    bt_info_len = file.tellg();
    if (bt_info_len <= 0) {
      fprintf(stderr, "file is empty\n");
      continue;
    }
    file.seekg(0, std::ios::beg);
    file.read(bt_info, bt_info_len);
    bt_info[bt_info_len] = '\0';
    if (!Json::String2Json(bt_info, bt_info_jv)) {
      fprintf(stderr, "invalid bt info format:\n");
      continue;
    }
    bool save_log = false;
    JV_TO_BOOLEAN(bt_info_jv, "save_log", save_log, false);
    if(save_log == false) {
      continue;
    }
    std::string thread_name;
    std::string crash_signal;
    std::string process_name;
    JV_TO_STRING(bt_info_jv, "thread_name", thread_name, "");
    JV_TO_STRING(bt_info_jv, "crash_signal", crash_signal, "");
    JV_TO_STRING(bt_info_jv, "process_name", process_name, "");
    snprintf(str, len,"Prog %s, thread %s recv signal %s", process_name.c_str(),
                                           thread_name.c_str(), crash_signal.c_str());
    // re-write to false
    bt_info_jv["save_log"] = false;
    bt_info_str = bt_info_jv.toStyledString();
    file.clear();
    file.seekg(0, std::ios::beg);
    file.write(bt_info_str.c_str(), bt_info_str.size());
    file.close();
    return true;
  }
  return false;
}
bool backtrace_clean_dump() {
  char filename[256];
  char bt_info[2048];
  std::string bt_info_str;
  size_t bt_info_len;
  Json::Value bt_info_jv;

  for (int i = 0; i < BACKTRACE_FILE_TOT; i++) {
    sprintf(filename, bt_file_format, i);
    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) {
      fprintf(stderr, "open %s failed.\n", filename);
      continue;
    }

    file.seekg(0, std::ios::end);
    bt_info_len = file.tellg();
    if (bt_info_len <= 0) {
      fprintf(stderr, "file is empty\n");
      continue;
    }

    file.seekg(0, std::ios::beg);
    file.read(bt_info, bt_info_len);
    bt_info[bt_info_len] = '\0';
    if (!Json::String2Json(bt_info, bt_info_jv)) {
      fprintf(stderr, "invalid bt info format:\n");
      continue;
    }

    if(bt_info_jv["is_new"] == false) {
      continue;
    }

    // re-write to old
    bt_info_jv["is_new"] = false;
    bt_info_str = bt_info_jv.toStyledString();

    file.clear();
    file.seekg(0, std::ios::beg);
    file.write(bt_info_str.c_str(), bt_info_str.size());
    file.close();
    break;
  }

  remove(wdg_file);
  return true;
}


bool backtrace_get_dump(char* dump, uint32_t dump_size, char* wdg, uint32_t wdg_size) {
  size_t ret;
  FILE *fp =NULL;
  char filename[256];
  std::string bt_info_str;
  size_t bt_info_len;
  Json::Value bt_info_jv;
  
  wdg[0] = '\0';
  dump[0] = '\0';
  for(int i = 0; i < BACKTRACE_FILE_TOT; i++) {
    sprintf(filename, bt_file_format, i);
    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) {
      fprintf(stderr, "open %s failed.\n", filename);
      continue;
    }

    file.seekg(0, std::ios::end);
    bt_info_len = file.tellg();
    if (bt_info_len <= 0) {
      fprintf(stderr, "file is empty\n");
      continue;
    }

    file.seekg(0, std::ios::beg);
    file.read(dump, dump_size);
    dump[bt_info_len] = '\0';
    if (!Json::String2Json(dump, bt_info_jv)) {
      fprintf(stderr, "invalid bt info format\n");
      continue;
    }

    // check new
    bool is_new = false;
    JV_TO_BOOLEAN(bt_info_jv, "is_new", is_new, false);
    if (!is_new) {
      memset(dump, 0, dump_size);
      continue;
    }
    file.close();
    break;
  }

  fp = fopen(wdg_file, "r");
  if(fp != NULL) {
    ret = fread (wdg, 1, (wdg_size - 1), fp);
    if (ret > 0) {
      wdg[ret] = '\0';
    }
    fclose(fp);
  }
  if ((strlen(wdg) >0) || (strlen(dump) > 0)) {
    return true;
  }
  return false;
}



#endif
