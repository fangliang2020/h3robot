

#include "daemon_service.h"
#include "common_ipc.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <glib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "db_manager.h"

struct process {
  char *name;
  char *args;
};


#define IPC_PROCESS_NUM 5 
static  struct process ipc_process_list[IPC_PROCESS_NUM] = {
    // {(char *)"log_server",(char *)""},
    {(char *)"dp_server",(char *)""},
    {(char *)"db_server_app",(char *)""},
    {(char *)"net_server_app",(char *)""},
    {(char *)"peripherals_server_app",(char *)""},
    {(char *)"robot_schedule_app",(char *)""},
};

static int PROCESS_NUM = IPC_PROCESS_NUM;
static struct process *process_list = ipc_process_list;

static bool program_checkrunning(const struct process *service) {
  FILE *fp;
  char cmd[128], buf[1024];
  char *pLine;

  assert(service);
  assert(service->name);
  snprintf(cmd, sizeof(cmd), "/bin/ps -ax| /bin/grep  %s | /bin/grep -v grep",
           service->name);
  fp = popen(cmd, "r");
  if (!fp)
    return false;
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    pLine = strtok(buf, "\n");
    while (pLine) {
      if (strstr(pLine, service->name)) {
        pclose(fp);
        return true;
      }
      pLine = strtok(NULL, "\n");
      //第二次调用strtok时候，传入参数应该为NULL，使得该函数默认使用上一次未分割完的字符串继续分割。
    }
  }
  pclose(fp);
  return false;
}

static void program_run(const struct process *service) {
  char cmd[64] = {0};

  assert(service);
  assert(service->name && service->args);
  snprintf(cmd, sizeof(cmd), "%s %s &", service->name, service->args);
  printf("start: %s\n", cmd);
  system(cmd);
}

static void program_kill(const struct process *service) {
  char cmd[64] = {0};

  assert(service);
  assert(service->name && service->args);

  snprintf(cmd, sizeof(cmd), "killall %s", service->name);
  system(cmd);
}

static void program_force_kill(const struct process *service) {
  char cmd[64] = {0};

  assert(service);
  assert(service->name && service->args);

  snprintf(cmd, sizeof(cmd), "killall -9 %s", service->name);
  system(cmd);
}

static gboolean program_check_and_run(gpointer user_data) {
  for (int i = 0; i < PROCESS_NUM; i++) {
    if (!program_checkrunning(&process_list[i]))
    {
      program_run(&process_list[i]);
      usleep(5000);
    }
  }
  return TRUE;
}

static gint timeout_tag;

int daemon_services_init() {
  PROCESS_NUM = IPC_PROCESS_NUM;
  process_list = ipc_process_list;
  return 0;
}

int daemon_services_start(unsigned int timer_ms) {
  program_check_and_run(NULL);

  // if ( timer_ms != 0 ) {
	//   timeout_tag = g_timeout_add(timer_ms, program_check_and_run, NULL);
  // } else {
	//   timeout_tag = 0;
  // }
  return 0;
}

int daemon_services_stop(void) {
  const int wait_time_ms = 100;
  int wait_count = 10;

  // if (timeout_tag) {
	//   g_source_remove(timeout_tag);
	//   timeout_tag = 0;
  // }

  while (--wait_count > 0) {
    /* killall process */
    for (int i = 0; i < PROCESS_NUM; i++) {
      if (program_checkrunning(&process_list[i]))
        program_kill(&process_list[i]);
    }
    usleep(wait_time_ms * 1000);
    /* check process */
    // bool succeed = true;
    int i;
    for (i = 0; i < PROCESS_NUM; i++)
      if (program_checkrunning(&process_list[i]))
        break;
    if (i == PROCESS_NUM)
      break;
  }

  /* if failed to killall process, force killall proces */
  if (wait_count == 0) {
    for (int i = 0; i < PROCESS_NUM; i++) {
      if (program_checkrunning(&process_list[i]))
        program_force_kill(&process_list[i]);
    }
    usleep(100 * 1000);
  }
  return 0;
}
