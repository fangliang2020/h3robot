/*
 * mqttprotocol.h
 *
 * This file is used to Test mqtt
 *      Author: pcwe2002
 */

#ifndef MQTTPROTOCOL_H_
#define MQTTPROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "protocol.h"
//#include "mutex.h"
#ifndef __RTOS__ 
#include <pthread.h>
#endif

#define MQTT_NAME  "MQTT"
#define	WILL_MSG "OFFLINE"
#define	WILL_MSG_TIPIC  "warning"

#define PROMPT_KEY_ERROR "00"
#define DEVICE_BIND		2
#define DEVICE_UNBIND	3
#define	DEVICE_MODIFY_NICKNAME	4




#define PREFIX	"d/"
#define SUFFIX	"/i"
#define DEV_MSG_SUFFIX	"/m"
#define DEV_STATUS_SUFFIX	"/s"

//物联网设备建议2048
#define BUFFER_SIZE 1024 * 128
    
#define TOPIC_MAX_SIZE 64
#define CID_MAX_SIZE 32
#define HTTP_RETURN 256

#define BEGIN_TOPIC_PIPELINE "pipeline"

//#ifndef DEBUG
//#define DEBUG
//#endif


#ifndef DEBUG
#define DEVICE_UPDATE_SECRET_TOPIC	"pipeline/secret"
#define DEVICE_UPLOAD_SDKINFO_TOPIC		"pipeline/sdkinfo"
#define DEVICE_ACTIVE_TOPIC				"pipeline/active"
#define APP_GET_SECRET_TOPIC		"pipeline/getsecret"
#define APP_GET_DEVICELIST_TOPIC	"pipeline/dlist"
#define DEVICE_GET_GROUPINFO        "pipeline/groupinfo"
#else
#define DEVICE_UPDATE_SECRET_TOPIC	"devpipeline/secret"
#define DEVICE_UPLOAD_SDKINFO_TOPIC	"devpipeline/sdkinfo"
#define DEVICE_ACTIVE_TOPIC			"devpipeline/active"
#define APP_GET_SECRET_TOPIC		"devpipeline/getsecret"
#define APP_GET_DEVICELIST_TOPIC	"devpipeline/dlist"
#define DEVICE_GET_GROUPINFO        "devpipeline/groupinfo"
#endif
#define PIPELINE_RETURN_METHOD		"method"
#define PIPELINE_RETURN_CODE		"code"
#define PIPELINE_RETURN_DATA		"data"
#define PIPELINE_RETURN_MSG         "msg"
#define PIPELINE_RETURN_SECURET_SKEY	"skey"
#define PIPELINE_RETURN_SECURET_SVALUE	"svalue"
#define PIPELINE_RETURN_DATA_LIST		"datalist"
#define PIPELINE_SUCCESS_CODE		200
#define ID_BYTES_NUMBER 	4

#define DEVICE_MAC_LEN		32
#define DEVICE_SOFT_VERSION_LEN		32
#define DEVICE_HARD_VERSION_LEN		32

#ifndef MANAGER_CONTROL
#define MANAGER_CONTROL
#endif

typedef struct _mdevice_t{
	device_t sdev;
	char isSecrect;	//是否加密设备； 1-加密设备、 0-不加密设备
	char isOnline;	//是否上线；1-上线、0-不在线
    char topic[30];
}mdevice_t;

typedef struct _ssl_context ssl_context;

typedef struct _mqttp_t mqttp_t;

typedef void (*MqttEvent)(int code, void *data, size_t len);

#ifndef MQTTEVENT_
#define MQTTEVENT_
    enum MQTTEVENT {
        MQTT_CONNECTED = 1,MQTT_DISCONNECTED,MQTT_KICKOUT
    };
#endif
#ifndef DEVICEEVENT_
#define DEVICEEVENT_
    enum DEVICEEVENT {
    	DEVICE_BIND_CODE = 1000, DEVICE_UNBIND_CODE, DEVICE_MODIFY_NICKNAME_CODE, DEVICE_LIST_REFRESH
    };
#endif
    
void *mqtt_init(const char *serverurl, int port, const char *username, const char *password);
void mqtt_destroy(void *mqtt);
void mqtt_set_device_param(mqttp_t *mqtt, const char *domain, const char* deviceURL, char bGetToken, const char *topic);

/**
 * 设置以客户端启动时mqtt的设备类型，设备如果需要通过影子设备与控制端交互，需要设置该值
 *
 * @param devType		在物模型设置的设备类型
 */
void mqtt_set_device_type(mqttp_t *mqtt, const char* devType);
    
void mqtt_set_name_pwd(mqttp_t *mqtt, const char *username, const char *password);
void mqtt_set_callback(mqttp_t *mqtt, MqttEvent event);

int piple_send_msg(const void* data, size_t length, char *sendTopic, int timeout);

int device_active(mqttp_t* mqtt, const char* data, int timeout);

void mqtt_set_key_param(mqttp_t *mqtt, const char *domain, const char* keyUpdateUrl, const char* keyGetUrl);
void mqtt_set_whole_appliance_param(mqttp_t *mqtt, const char *voiceDomain, const char* groupInfoUrl, const char* uploadInfoUrl);
void mqtt_set_get_device_list_or_not(mqttp_t * mqtt, int flag);
void mqtt_set_infos_changed_callback(mqttp_t *mqtt,void *w, void *callback);
void * encrypt_data( const void *data, size_t length, size_t *size);

#ifdef __cplusplus
}
#endif

#endif /* MQTTPROTOCOL_H_ */
