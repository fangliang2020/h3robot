/*
 * @描述:
 * @版本:
 * @作者:
 * @Date: 2021-11-20 15:48:28
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2021-12-11 16:11:28
 */
#ifndef _APP_FOLLOWWALL_H
#define _APP_FOLLOWWALL_H

//#include "../global/types.h"
//#include "../global/tools.h"
//#include "../global/debug.h"
#include "apptypes.hpp"
#include "fsm.hpp"
#include "map.hpp"
#include "vitual_wall.hpp"
#include "cmd_interface.hpp"
#include "config.hpp"
#include "dwa.hpp"
#include "robot_schedule/socket/navigate.h"


typedef struct follow_wall_params
{
	long back_time ;//用于保存后退时间
	long turn_time;
	int turn_angle ; //需要转的角度
	int target_angle ;//目标角度
	int target_path_index;//目标点数组的索引
	int turn_direct;//转弯的方向 0 左转 1 右转
	int start_angle ;//保存其实角度
	int soft_turn;//用于在impact_turn90_event区分是硬碰撞还是软碰撞 0:硬碰撞,1:软碰撞
	int hard_impact; //用于硬碰撞发生后进行转弯
	int turn_flag ;//判断当前是不是在转弯,0不是,1是	
	int find_wall_turn_direct;//去指定地点时,发生碰撞后,传给红外沿物使用左红外还是右红外
	int ir_impact;//红外发生碰撞1:是,0不是
	//计算方程的系数Ax+By+C=0
	int a ;
	int b ;
	int c ;
	Point *coordinates; //当前的坐标
} FW_PARAMS; 

typedef enum fw_mode_e
{
	FIND_WALL = 0,				//找墙
	FOLLOW_WALL,				//走N*N的小方格
	BACK,						//走N*N碰到障碍后退
	BACK1,						//STRAIGHT状态发生碰撞后退,和back一样但是切换的状态不一样
	TURN,						//碰到障碍转弯
	TURN1,						//用于在沿墙信号丢失后进行调整
	ERROR,						//发生错误
	AVOID,						//绕障碍物
	FINISH,						//走N*N的小方格回到初始点后结束
	FIND_BACK,					//找墙的时候发生碰撞,后退
	FIND_TURN,					//后退后转弯
	FIND_AVOID,					//转完后进行沿墙
	GET_STRIGHT_LINE,			//开机取直线
	INIT,
	CIRCLE,						//开机找直线后转
	OUT_BASE_STATION,			//从基站中出来
	FOLLOW_FORBIDEN_ZONE,
	CLEAN_MOP,
	NEXT_ZONE,
	CHECK_FOLLOW_TYPE,
} FW_MODE_E;
typedef struct followwall_state
{
	FW_MODE_E cur_state;
	void (*ctrlfunc)(void);
	void (*ctrlinit)(void);
} FOLLOWWALL_STATE;

typedef struct followwall_event
{
	FW_MODE_E cur_state;
	bool (*event)(void);
	FW_MODE_E next_state;
} FOLLOWWALL_EVENT;

typedef struct zone_data_distance{
	Point point;
	double distance;
}ZONE_DATA_DISTANCE;

enum FollowWallAction { AdjustOri = 0, DynamicLineMove, StaticLineMove, AroundPointMove, Default};
extern enum FollowWallAction prev_action_;
extern FW_MODE_E gfw_state;
extern Rectangle zone_box;
extern Point zone_start_point;
extern Point change_search_point;
extern int follow_obstacle;
extern bool block_init;
extern SOCKET_ZONE_DATA zone_data;

void follow_var_reset();
void followwall_init();
void followwall_perform();
void followwall_zone_download(int startX,int startY,double yaw,int rect0_X,int rect0_Y,int rect1_X,int rect1_Y);
void follow_method_init(int direct);
void follow_method();
void follow_front_obstacle(int cmd);
bool over_border_event();
int arrival_target_point(Point targetPoint,Point now);
int get_atan(Point now , Point target_point);

void set_fw_coordinates(int x,int y);
void api_straight(int err_angle,int err_dist);
void cmd_download(U8 cmd);
void set_pitch_angle(int pitch_angle);
void check_findwall_circle_init();
int revert_to_360(int angle);
bool lost_signal();
#endif // EOF _APP_FOLLOWWALL_H