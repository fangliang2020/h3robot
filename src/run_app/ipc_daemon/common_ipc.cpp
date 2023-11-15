
#include "common_ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


static int system_cmd(const char *cmd) {
  pid_t status;

  if (!cmd)
    return -1;

  printf("%s: %s\n", __func__, cmd);

  status = system(cmd);

  if (-1 == status) {
    printf("system error!");
  } else {
    if (WIFEXITED(status)) {
      if (0 == WEXITSTATUS(status)) {
        return 0;
      } else {
        printf("run shell script fail, script exit code: %d\n",
              WEXITSTATUS(status));
      }
    } else {
      printf("exit status = [%d]\n", WEXITSTATUS(status));
    }
  }
  return status;
}

int copy(const char *src, const char *dst) {
  if (!src || !dst)
    return -1;

  printf("%s: %s -> %s\n", __func__, src, dst);

  if (!access(src, F_OK)) {
    char cmd[128] = {'\0'};
    snprintf(cmd, 127, "cp %s %s\n", src, dst);
    return system_cmd(cmd);
  } else {
    printf("File %s does not exist\n", src);
    return -1;
  }
}
