
#ifndef __DB_MANAGER_H__
#define __DB_MANAGER_H__

// #ifdef __cplusplus
// extern "C" {
// #endif

#define SYSCONFIG_DB_DEFAULT_PATH	(char *)"/oem/cfg/sysConfig.db"
#define SYSCONFIG_DB_RUNNING_PATH	(char *)"/oem/cfg/run/sysConfig.db"
#define SYSCONFIG_DB_BACKUP_PATH	(char *)"/oem/cfg/backup/sysConfig.db"

int db_manager_init_db(void);
int db_manager_reset_db(void);
int db_manager_import_db(char *db_path);
int db_manager_export_db(char *db_path);

// #ifdef __cplusplus
// }
// #endif

#endif
