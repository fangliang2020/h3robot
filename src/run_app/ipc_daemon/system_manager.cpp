#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#include "system_manager.h"
#include "db_manager.h"
#include "daemon_service.h"


int system_upgrade() {
  char cmd[128] = {'\0'};

  /* TODO: check firmware */
  if(access("/userdata/update.img", F_OK)!=0) return 0;

  snprintf(cmd, 127, "updateEngine --image_url=/userdata/update.img --misc=update --savepath=/userdata/update.img --reboot &");

  system(cmd);
  return 1;
}

int factory_reset()
{
  daemon_services_stop();
  db_manager_reset_db();
  system("reboot"); //system("update");
  return 0;
}
