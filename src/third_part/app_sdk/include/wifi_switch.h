#ifndef _WIFISWITCH_H_
#define _WIFISWITCH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
typedef struct{

	/*
	  * ssid's content has 3 types: type-1 and type-2 is used by chong, 
	  *  						  and type-3 is used by other platform.
	  *1: MAC + ssid
	  *2: MAC + UTF-8 len + UTF-8 + GBK len + GBK
	  *3: ssid
	  */
	unsigned char type; 

	unsigned char ssid_len;
	unsigned char pwd_len;
	
	unsigned char *ssid;
	unsigned char *pwd;

}config_info_t;

typedef struct{    
	unsigned char ssid_len ;
    unsigned char password_len;
	unsigned char bssid[7] ; //mac
	unsigned char ssid[34];
	unsigned char password[65] ;
}network_t;

typedef void (*wifi_info_callback_t)(void *v);

void set_wifi_info_callback(wifi_info_callback_t wifi_cb);
void *chong_ap_config_task(void *para);
void ch_wifi_apstart(char *sn);//待配网起热点
void ch_wifi_apstop(void);//关闭待配网热点
int regist_device_new(uint8_t *HexKey,char *sn);//正式服注册
int regist_device_test(uint8_t *HexKey,char *sn);//测试服注册
int httpNewUnBind(uint8_t *HexKey,char *sn);//正式服解绑
int httpNewUnBindTest(uint8_t *HexKey,char *sn);//测试服解绑
int wifi_connect(char *ssid,char *password);//联网

#ifdef __cplusplus
}
#endif
#endif

