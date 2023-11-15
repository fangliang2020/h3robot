
#include <stdlib.h>
#include <unistd.h>


#include "common_ipc.h"
#include "db_manager.h"

int db_manager_init_db(void) {
  //如果检测不到运行的sysconfig.db,则把backup中的拷贝过去
  if (access(SYSCONFIG_DB_RUNNING_PATH, F_OK)!=0) { 
    printf("no SYSCONFIG_DB_RUNNING_PATH\n");
    if(access(SYSCONFIG_DB_BACKUP_PATH, F_OK)!=0) //如果也没有备份的数据
    {
      printf("no SYSCONFIG_DB_BACKUP_PATH\n");
      copy(SYSCONFIG_DB_DEFAULT_PATH, SYSCONFIG_DB_BACKUP_PATH);
      copy(SYSCONFIG_DB_DEFAULT_PATH, SYSCONFIG_DB_RUNNING_PATH);
      return 0;
    }
    else {
      copy(SYSCONFIG_DB_BACKUP_PATH, SYSCONFIG_DB_RUNNING_PATH);
      return 0;
    } 
  }
  //如果存在运行的sysconfig.db，则把运行中的db拷贝到备份backup中
  /* copy defaut db to runtime db */
  printf("yes SYSCONFIG_DB_RUNNING_PATH\n");
  return copy(SYSCONFIG_DB_RUNNING_PATH, SYSCONFIG_DB_BACKUP_PATH);
}

int db_manager_reset_db(void) {
  /* copy defaut db to runtime db */
  copy(SYSCONFIG_DB_DEFAULT_PATH, SYSCONFIG_DB_BACKUP_PATH);
  return copy(SYSCONFIG_DB_DEFAULT_PATH, SYSCONFIG_DB_RUNNING_PATH);
}

int db_manager_import_db(char *db_path) {
  /* TODO: check db*/

  /* copy new db to runtime db */
  return copy(db_path, SYSCONFIG_DB_RUNNING_PATH);
}

int db_manager_export_db(char *db_path) {
  /* copy runtime db to export db */
  return copy(SYSCONFIG_DB_RUNNING_PATH, db_path);
}
