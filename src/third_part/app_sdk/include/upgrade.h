#ifndef UPGRADE_H_INCLUDED
#define UPGRADE_H_INCLUDED

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h> 
//#include "gloableDef.h"
#define VERSION_BUF_SIZE 15
#define DEVICE_TYPE_BUF_SIZE 12
#define SOC_TYPE_BUF_SIZE 12
#define DEVICE_MODEL_BUF_SIZE 34
#define DEVICE_NAME_BUF_SIZE 52
#define PATH_PREFIX_BUF_SIZE 100

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
	u_false=0,
	u_true
}upgrade_bool_t;

//refer to the protocol for communication between device and cloud server
//must be the same with protocol definition
typedef enum {
	response_pachage_check_erro=1,
	response_starting_upgrade,
	response_upgraded_success,
	response_upgraded_fail
}response_type_t;

//refer to response_type_t above
typedef enum {
	upgraded_success=3,
	upgraded_fail=4,
	upgraded_no_upgrade=5
}upgrade_result_t;

typedef enum{
	upgrade_force=0,
	upgrade_with_prompt
}upgrade_type_t;

typedef struct {
	char hardware_version[VERSION_BUF_SIZE]; //we assume caller will give the format "x.y.z", e.g "1.0.0"
	char software_version[VERSION_BUF_SIZE];	  //we assume caller will give the format "x.y.z", e.g "1.0.0"
	char device_type[DEVICE_TYPE_BUF_SIZE];
	char soc_type[SOC_TYPE_BUF_SIZE];
	char device_model[DEVICE_MODEL_BUF_SIZE];
	char device_name[DEVICE_NAME_BUF_SIZE];  //if Chinese character is used ,please make sure  econded as UTF-8
	char sn[50];
	char path_prefix[PATH_PREFIX_BUF_SIZE]; 
	upgrade_bool_t web_ui_enable;	

}upgrade_para_t;

upgrade_bool_t config_upgrade_para(const upgrade_para_t *upgrade_cfg,char *path_prefix);
upgrade_bool_t init(void);
upgrade_bool_t is_downloading(void);
upgrade_bool_t download(void);
upgrade_bool_t have_new_version(char *softversion, char *hardversion);
void apply_upgrade(void);//will cause system to reboot
int get_check_times(void);
int get_polling_time(void);
upgrade_bool_t  respons_upgrade_result_to_cloude_server(response_type_t result);
upgrade_type_t get_upgrade_type(void);
upgrade_result_t  check_last_upgrade_status(void);
void remove_download_status_files(void);
void *u_upgrde(void *ptr);

#ifdef __cplusplus
}
#endif
//the caller shoud call 'config_upgrade_para' first to set the current version info
//the 'check_last_upgrade_status' will compare the versions before upgrading and current
//if the same, then return upgraded_success, else return upgraded_fail
#endif // UPGRADE_H_INCLUDED

