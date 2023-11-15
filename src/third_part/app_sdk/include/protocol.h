/*
 * protocol.h
 *
 *  Created on: 2015
 *      Author: pcwe2002
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef __RTOS__
#include "MQTTfreeRTOS.h"
#endif

typedef void (*SmartPEvent)(void */*p*/, const void*/*data*/, size_t /*length*/, void *arg);
typedef void (*SmartPCommandEvent)(void */*p*/, uint16_t nid, const void*/*data*/, size_t /*length*/, void *arg);


#define FLAG_MANAGER 1
#define FLAG_DEVICE 2
#define FLAG_ALL 3

#define PROTOCOL_MQTT 1
#define PROTOCOL_LAN 2

#define MAX_SN_LEN 64
#define SECRET_KEY_LEN 16
    
enum COMMANDTYPE {
    	COMMAND_ONLY_ROUTER = 0, COMMAND_THROW_SHADOW
    };
    
typedef struct _protocol_t protocol_t;

typedef  struct _device_t{
	char sn[MAX_SN_LEN];
	char name[MAX_SN_LEN];
	uint8_t secretkey[SECRET_KEY_LEN];
    uint8_t flag;           //区分manager和device,
	protocol_t *protocol;
	//设备大类
    char devType[MAX_SN_LEN];
    //IOT平台配置的标识
    char type[MAX_SN_LEN];
} device_t;

typedef struct {
	char *serverUrl;
	int port;
} p_config_t;

typedef int  (*StartServerFlag)( protocol_t* /*protocol*/,const char* /*uuid*/, int /*flag*/);
typedef int  (*StartServer)( protocol_t* /*protocol*/,const char* /*uuid*/);
typedef int  (*StopServer)(protocol_t* /*protocol*/);
typedef int  (*SendToAll)(protocol_t* /*protocol*/, const void* /*data*/, size_t /*length*/ );
typedef int  (*SendCommand)(device_t */*device*/, const void */*data*/, size_t /*length*/);
typedef int  (*ReplayCommand)(device_t */*device*/, uint16_t nid, const void */*data*/, size_t /*length*/);
typedef int  (*SendCommandReceived)(device_t */*device*/, const void */*data*/, size_t /*length*/, void */*recData*/, size_t */*recLen*/, int /*timeout*/);
//typedef int  (*ReplayCommand)(device_t *device, uint16_t nid, void *data, size_t size);
typedef void (*AddListenDevice)(protocol_t* /*protocol*/, const char */*deviceSN*/);
typedef void (*RemoveListenDevice)(protocol_t* /*protocol*/, const char */*deviceSN*/);

typedef void (*AddDeviceKey)(protocol_t* /*protocol*/, const char */*deviceSN*/, const char */*secretKey*/);

/**
 * 发送命令不和影子设备交互
 */
typedef int  (*SendCommandByRouter)(device_t */*device*/, const void */*data*/, size_t /*length*/, int flag);
/**
 * 发送命令查询设备状态
 */
typedef int  (*SendCommandQuery)(device_t */*device*/);
/**
 *
 * 设备发送状态通过路由不和影子设备交互
 */
typedef int  (*SendStatusByRouter)(protocol_t* /*protocol*/, const void* /*data*/, size_t /*length*/, int flag );
/**
 *
 * 设备对固定的手机回复消息
 */
typedef int  (*SendStatusToSpecific)(device_t */*device*/, const void* /*data*/, size_t /*length*/ );

/*
 *
 *   int StartServer( protocol_t* protocol,const char* uuid);
 *   int StopServer(protocol_t* protocol)
 *   int SendStatus(protocol_t* protocol, const char* data, int length )
 *   int SendCommand(device_t *device, const char *data, int length);
 *   int SendCommandReceived(device_t *device, const char *data, int length, char *recData, int* recLen, int timeout)
 */

//typedef struct _smartp_t smartp_t;

typedef struct _protocol_t{
	const char *name;			//实现协议时静态存储区的名称给此变量，如static或全局
	//smartp_t * smartp;          //协议框架
    void *arg;
    uint8_t priority;           //优先级
    uint8_t secrt;              //是否加密，如果为0,则smartp会加密，为1则不需要smartp加密
    
    StartServerFlag start_server;
    StopServer stop_server;
    
    StartServer device_start_server;
	StopServer device_stop_server;

	SendToAll send_to_all;
	SendCommand send_command;					//发送命令不等待接收
	SendCommandReceived send_command_received;	//发送命令等待接收
	ReplayCommand replay_command;			//返回相应命令

	StartServer manager_start_server;
	StopServer manager_stop_server;
	AddListenDevice manager_add_listen_device;
	RemoveListenDevice manager_remove_listen_device;
	AddDeviceKey manager_add_device_key;

	SmartPCommandEvent onCommandReceived;
	SmartPEvent onDeviceAdd;
	SmartPEvent onDeviceRemove;

	/*
	 * IOT new SDK add
	 */
	SendCommandByRouter manager_send_command_by_router;
	SendStatusByRouter device_send_status_by_router;
	SendStatusToSpecific device_send_status_specific;
	SendCommandQuery manager_send_command_query;

} protocol_t;

#ifdef  __cplusplus
}
#endif

#endif /* PROTOCOL_H_ */
