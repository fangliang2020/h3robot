#ifndef _APP_FSM_H
#define _APP_FSM_H
#include <unistd.h>
#include "robot_schedule/socket/thread_pool.h"
#include "apptypes.hpp"
#include "followwall.hpp"
#include "zmode.hpp"
#include "map.hpp"
#include "vitual_wall.hpp"
#include "cmd_interface.hpp"
#include "QuickMapping.hpp"

#define NOT_CHECK_BASE 0
extern bool start_from_base;

typedef struct follow_params
{
	int turn_direct;  	//0-左,1-右
	int ir;				//当前沿墙信号
	int max_ir;
	int last_ir;		//上一次沿墙信号
	int speed[2];
	int wall_speed;
	int a ;
	int b ;
	int c ;
	int target_angle;
}FOLLOW_PARAMS;
extern int gAngle;			//直角坐标系下,全局机器偏航角,90度为正前方(提高精度,乘以100,比如120度,则输出为12000)
extern Point *coordinates;	//全局机器坐标[mm,mm]
// extern dwi_res_group dwi_vol;
extern int robot_location;
extern Rectangle charge_zone;


typedef struct topmode
{
	TOPMODE_E cur_state;	//状态
	void (*ctrlinit)(void);
	void (*ctrlfunc)(void);	//处理函数(通常为循环)
} TOPMODE;

typedef enum STATE_SHIFT{
	DEFAULT,
	//需要先返回基站然后再出来
	AGAIN_CLEAN,
	//返回基站不出来
	RETURN_BASE,
	CONTINUE_CLEAN,//断点续扫
}STATE_SHIFT;

//对雷达进行休眠和唤醒操作
typedef enum SLAM_STATE{
	SLEEP,
	AWAKE,
}SLAM_STATE;

typedef enum ASR_MSG{
	ASR_SILENCE,
	ASR_START,
	ASR_PAUSE,
    ASR_BACK_CHARGE,
	ASR_CLEAN_LIVING_ROOM,
	ASR_CLEAN_DINNING_ROOM,
	ASR_CLEAN_BATH_ROOM,
}ASR_MSG;
//



void set_ML_clean(char cmd);
void set_ML_pause(int cmd);
void set_ML_room(char cmd);
void set_ML_init_state(char cmd);
void set_ML_load_map_id(char map_id);
void set_ML_continue_clean(char continue_clean);

bool load_map_and_relocate();
void asr_msg_download(ASR_MSG msg);
void modeover_callback(TOPMODE_E needMode);
void set_base_cmd(int cmd);
uint8_t control_thread();
int get_now_state();
void awake_or_sleep_slam(SLAM_STATE cmd);
void reset_robot_state();
void pose_download(int x,int y,double angle);
int get_clean_time();
uint8_t query_now_mode();


#endif	//EOF _APP_HANDLE_H