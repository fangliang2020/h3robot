#include <stdio.h>
#include "core_include.h"
#include "eventservice/util/mlgetopt.h"
#include "eventservice/util/ml_pipe.h"
#include "eventservice/util/proc_util.h"
#include "eventservice/util/time_util.h"
#include "eventservice/util/file_opera.h"
#include "eventservice/util/sys_info.h"

static uint32_t g_opensdk_mem = 0;

int softdog_enable = 1;

extern void backtrace_init();

void SetDog(const void *vty_data, const void *user_data,
            int argc, const char *argv[]) {
  if (argc != 2) {
#ifdef ENABLE_VTY
    // Vty_Print(vty_data, "Useage : set_dog 0/set_dog 1\n");
#endif
    return;
  }
  bool* is_pass_check_module = (bool*)(user_data);
  if (0 == strcmp(argv[1], "0")) {
    *is_pass_check_module = true;
  } else if (0 == strcmp(argv[1], "1")) {
    *is_pass_check_module = false;
  }
}

void CheckUsrAppMem(int sec) {
  static int usrapp_interval = 60 * 2;  // 2min

  if (g_opensdk_mem == 0) {
    return;
  }

  usrapp_interval -= sec;
  if (usrapp_interval > 0) {
    return;
  }
  uint32_t usrapp_mem = UsrAppMemUsed();
  //if usrapp used more than 20MB
  if (usrapp_mem > g_opensdk_mem * 1024) {
    DLOG_ERROR(MOD_EB, "usr app out of memory %u", usrapp_mem);
    LOG_TRACE(LT_SYS, "%04d usr app out of memory%u", LOG_ID_USRAPP_MEM, usrapp_mem);
    usrapp_interval = 60 * 10;
  }
  else {
    DLOG_INFO(MOD_EB, "usr app use memory %u\n", usrapp_mem);
    fprintf(stderr, "usr app use memory %u\n", usrapp_mem);
    usrapp_interval = 60 * 2;
  }
}

#define ML_MEM_BUF_NUM (64)
static int g_meminfo_idx = 0;
static meminfo_t g_mem_info[ML_MEM_BUF_NUM];

void CheckSysMem(int sec) {
  static int mem_interval = 10;  // 10s
  
  mem_interval -= sec;
  if (mem_interval > 0) {
    return ;
  }

  mem_interval = 10;// 10s
  if (GetMemInfo(&g_mem_info[g_meminfo_idx]) == false) {
    return;
  }

  g_meminfo_idx++;
  if (g_meminfo_idx >= ML_MEM_BUF_NUM) {
    g_meminfo_idx = 0;
  }
}

bool SaveSysMem() {
  int i = 0;

  FILE* fp = fopen("/userdata/log/system_server_files/meminfo.log", "w");
  if (fp == NULL) {
    return false;
  }
    
  for (i = 0; i < ML_MEM_BUF_NUM; i++) {
    fprintf(fp, "%u,%u,%u\n", g_mem_info[i].ts, g_mem_info[i].freed, g_mem_info[i].cached);
  }

  fclose(fp);
  return true;
}

int main(int argc, char *argv[]) {
  printf("log compile %s %s\n", __DATE__, __TIME__);
  backtrace_init();

#ifdef USE_SYS_LOG
  sys_log_init();
  set_syslog_level(LOG_ERR);
#endif

  // log
  bool bret = Log_Init(true);
  ASSERT_RETURN_FAILURE((false == bret), -1);
  
  bool is_pass_check_module = false;

  int32_t ch = 0, arg_num;
  while ((ch = ml_getopt(argc, argv, (char*)"p:w:h")) != EOF) {
    switch (ch) {
    case 'p': {
      sscanf(ml_optarg, "%d", &arg_num);
      printf("%s[%d] log output level %d\n", __FUNCTION__, __LINE__, arg_num);
      Log_DbgAllSetLevel(arg_num);
      break;
    }

    case 'w': {
      sscanf(ml_optarg, "%d", &arg_num);
      printf("%s[%d] watchdog %d\n", __FUNCTION__, __LINE__, arg_num);
      is_pass_check_module = (arg_num == 0) ? true : false;
      break;
    }
    
    case 'h':
      break;
    }
  }

  // log_server启动完后再运行runserver脚本
  // TODO system("./runserver &");
  const int sec = 5;
  while (true) {
    chml::Thread::SleepMs(sec * 1000);
    CheckSysMem(sec);
    CheckUsrAppMem(sec);
  }

  return 0;
}
