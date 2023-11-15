/*
 * smartp.h
 *
 *  Created on: 2015
 *      Author: pcwe2002
 */

#ifndef SMARTP_H_
#define SMARTP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "protocol.h"

#ifdef __RTOS__
#include "MQTTfreeRTOS.h"
#endif

typedef struct _smartp_t smartp_t;

    
/**
 * create smart protocol
 * @return
 */
smartp_t* smartp_create(void *arg);

/**
 * destroy smart protocol, clean the resource
 * @param smartp
 */
void smartp_destroy(smartp_t* smartp);

/**
 * register the protocol, should be register more than one protocol
 *    before start server
 * @param smartp		pointer of smartp
 * @param protocol		pointer of protocol
 */
void smartp_protocol_register(smartp_t *smartp,protocol_t *protocol);

/**
 * set the callback function for smartp
 * @param smartp
 * @param onDeviceAdd			it will be called when device found
 * @param onDeviceRemove		it will be called where device lost
 * @param onStatusChanged		status changed
 */
void smartp_set_callback(smartp_t* smartp, SmartPEvent onDeviceAdd,
		SmartPEvent onDeviceRemove, SmartPCommandEvent onCommandReceived);


/**
 * start server
 * @param smartp
 * @param sn			device sn
 * @param flag       FLAG_MANAGER FLAG_DEVICE FLAG_ALL
 * @return
 */
int smartp_start_server(smartp_t* smartp, const char* sn, int flag);

/**
 * stop device server
 * @param smartp
 * @return
 */
int smartp_stop_server(smartp_t* smartp);

/**
 * start device server
 * @param smartp
 * @param sn			device sn
 * @return
 */
int device_start_server(smartp_t* smartp, const char* sn);

/**
 * stop device server
 * @param smartp
 * @return
 */
int device_stop_server(smartp_t* smartp);

/**
 * set call back function
 * @param smartp
 * @param onCommandReceived		function will be called when command received
 */
void device_set_callback(smartp_t *smartp, SmartPCommandEvent onCommandReceived);

/**
 * send status data to all of manager
 * @param smartp
 * @param data        the status data
 * @param length      the length of data
 * @return
 */
int device_send_to_all(smartp_t *smartp, const void* data, size_t length );

/**
 * start manager server
 * @param smartp
 * @param sn				manager sn
 * @return
 */
int manager_start_server(smartp_t* smartp, const char* sn);

/**
 * stop manager server
 * @param smartp
 * @return
 */
int manager_stop_server(smartp_t* smartp);

/**
 * send command to device
 * @param smartp
 * @param device    device
 * @param data		the command data
 * @param length    the length of data
 * @return
 */
int smartp_send_command(device_t *device, const void *data, size_t length);

/**
 * send command to device and device return data to
 * @param smartp
 * @param device
 * @param data			the command data
 * @param length		the length of data
 * @param recData		received data
 * @param recLen		received length
 * @param timeout		timeout for millisecond
 * @return
 */
int smartp_send_command_received(device_t *device,
		const void *data, size_t length, void *recData, size_t *recLen, int timeout);

/**
 * replay command to device
 * @param device
 * @nid command id
 * @param data			the command data
 * @param length		the length of data
 * @return
 */
int  smartp_replay_command(device_t *device, uint16_t nid, const void *data, size_t length);

/**
 * Get Device Protocol Type
 * @param device
 * @return network type maybe PROTOCOL_MQTT | PROTOCOL_LAN
 */
int smartp_get_device_network_type(device_t *device);
    
/**
 * Add listen device(only for remote protocol like mqtt\xmpp )
 * the listen device will appear in onDeviceAdd
 * @param smartp
 * @param deviceSN
 */
void manager_add_listen_device(smartp_t *smartp, const char *deviceSN);

/**
 * Remove listen device the device will not listen
 * @param smartp
 * @param deviceSN
 */
void manager_remove_listen_device(smartp_t *smartp, const char *deviceSN);


void manager_add_device_secretkey(smartp_t *smartp, const char *deviceSN, const char *key);


/**
 * send command to device
 * @param device
 * @param data
 * @param length
 * @param flag 0 : 发送消息给路由不和影子设备交互
 * 					1 ： 发送消息给路由和影子设备交互
 */
int smartp_send_command_by_router(device_t *device, const void *data,size_t length,int flag);

/**
 * send command for query device status
 * @param device
 */
int smartp_send_command_query(device_t *device);

/**
 * send status from device to all
 * @param smartp
 * @param data
 * @param length
 * @param flag
 */
int device_send_by_router(smartp_t *smartp, const void* data, size_t length, int flag);

/**
 * send status from device to specific
 * @param device
 * @param data
 * @param length
 */
int device_send_to_specific(device_t *device, const void *data,size_t length);


#ifdef __cplusplus
}
#endif
#endif /* SMARTP_H_ */
