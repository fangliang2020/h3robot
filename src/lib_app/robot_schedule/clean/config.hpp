/*
#ifndef _CONFIG_H
#define _CONFIG_H
//#include "stdbool.h"
#include "apptypes.hpp"
#include "Peripheral/device_manager.h"
#include "../Socket/navigate.h"
//定义轮子速度控制(占空比)宏,app等外部调用
#define WHEEL_INIT_SPEED	20		//初值速度,3000/2万,其他速度不能小于这个值
#define WHEEL_SPEED_HIGH	300  	//最高速度,2万/2万=全速
#define WHEEL_SPEED_NORMAL	250     //弓字长边速度,5000/2万=1/4   8000
#define WHEEL_SPEED_UNIFORM 200
#define WHEEL_SPEED_TURN    180    //一个轮转一个轮不转
#define WHEEL_SPEED_MIDDLE  100  	//
#define WHEEL_SPEED_LOW		50	    //开机找直线的速度
#define WHEEL_SPEED_STOP	0					    	//停车
typedef struct Collision 
{
	bool l;
	bool r;
	bool ll;
	bool rr;
}Collision;
typedef unsigned char uint8_t;



extern float track_width_;
extern float max_angular_velocity_;
extern float far_away_small_obs_distance_thresh_;
extern float min_linear_velocity_;
extern float far_away_wall_distance_thresh_;
extern float follow_wall_sensor_offset_y_;
extern float follow_wall_dist_;
extern float tracking_line_thresh_;
extern float prev_linear_vel_;
extern float prev_angular_vel_;
extern float follow_wall_dt_;
extern float max_linear_acceleration_;
extern float max_angular_acceleration_;
extern float max_linear_velocity_;
//void setVWtoRobot(float v,float w);
Collision GetData();
uint8_t getTofdata();
void register_setVWtoRobot(void (*pf)(int16_t,int16_t));
//extern void (*send_cmd)(U8);
//extern void (*setVWtoRobot)(int16_t,int16_t);
void set_followwall_ir_data(int32_t ir_data);
void set_impact_ir_data(uint8_t la,uint8_t rb,uint8_t ra,uint8_t lb);
void register_send_cmd(void (*pf)(uint8_t));
void register_get_route(void (*pf)(Point,Point));
//extern void (*send_cmd)(U8);


enum base_e {
    CMD_RECV_DATA = 0,          //用于判断是否为机器主动发送消息    
    CMD_CLEAN_KEY,           //开始清扫，暂停清扫,基站上1个按钮
    CMD_RECALL_KEY,      //返回基站，离开基站,基站上1个按钮
    CMD_ROBOT_WHERE,            //查询机器是否在基站
    CMD_WASH_START,             //开始清洗拖布
    CMD_WASH_STOP,              //停止清洗拖布
    CMD_BASE_RESPONSE,				//基站响应洗拖布
    CMD_BLOW_OVER,
};
#endif
*/
#ifndef _CONFIG_H
#define _CONFIG_H
//#include "stdbool.h"
#include "apptypes.hpp"
//#include "Peripheral/device_manager.h"
#include "robot_schedule/socket/navigate.h"
#include "followwall.hpp"
//定义轮子速度控制(占空比)宏,app等外部调用
#define WHEEL_INIT_SPEED	20		//初值速度,3000/2万,其他速度不能小于这个值
#define WHEEL_SPEED_HIGH	300  	//最高速度,2万/2万=全速
#define WHEEL_SPEED_NORMAL	250     //弓字长边速度,5000/2万=1/4   8000
#define WHEEL_SPEED_UNIFORM 200
#define WHEEL_SPEED_TURN    180    //一个轮转一个轮不转
#define WHEEL_SPEED_MIDDLE  100  	//
#define WHEEL_SPEED_LOW		50	    //开机找直线的速度
#define WHEEL_SPEED_STOP	0					    	//停车

//触发硬碰撞
#define HardCollisionL  (get_impact_data().l == 1)	//1触发,0不触发
#define HardCollisionLC  (get_impact_data().lc == 1)	//1触发,0不触发
#define HardCollisionRC (get_impact_data().rc == 1)
#define HardCollisionR (get_impact_data().r == 1)
#define HardCollisionC (get_impact_data().lc == 1 && get_impact_data().rc == 1)
//未触发硬碰撞
//#define HardCollisionCX 0//(GetData().l == 0 && GetData().r == 0)
#define HardCollisionCX (get_impact_data().l == 0 && get_impact_data().r == 0 &&get_impact_data().lc == 0 && get_impact_data().rc == 0 )


extern float track_width_;
extern float max_angular_velocity_;
extern float far_away_small_obs_distance_thresh_;
extern float min_linear_velocity_;
extern float far_away_wall_distance_thresh_;
extern float follow_wall_sensor_offset_y_;
extern float follow_wall_dist_;
extern float tracking_line_thresh_;
// extern float prev_linear_vel_;
// extern float prev_angular_vel_;
// extern float follow_wall_dt_;
extern float max_linear_acceleration_;
extern float max_angular_acceleration_;
extern float max_linear_velocity_;
extern int32_t follow_wall_ir;
typedef struct Collision 
{
	uint8_t l;
	uint8_t r;
	uint8_t ll;
	uint8_t rr;
}Collision;

// Collision GetData();
// uint8_t getTofdata();
void register_send_cmd(void (*pf)(uint8_t));
void register_setVWtoRobot(void (*pf)(int16_t,int16_t));
void register_get_route(void (*pf)(Point,Point));
void set_robot_stop();
void register_followwall_maps_upload(void (*pf)(BLOCK_DATA *,int));
Collision get_impact_data();

//extern void (*send_cmd)(uint8_t);
//extern void (*setVWtoRobot)(int16_t,int16_t);
//extern void (*get_route)(Point,Point);

void set_followwall_ir_data(int32_t ir_data);
void set_impact_ir_data(uint8_t la,uint8_t rb,uint8_t ra,uint8_t lb);
void set_tof_data(int x,int y,uint16_t dis);

enum base_e {
    CMD_RECV_DATA = 0,          //用于判断是否为机器主动发送消息    
    CMD_CLEAN_KEY,           //开始清扫，暂停清扫,基站上1个按钮
    CMD_RECALL_KEY,      //返回基站，离开基站,基站上1个按钮
    CMD_ROBOT_WHERE,            //查询机器是否在基站
    CMD_WASH_START,             //开始清洗拖布
    CMD_WASH_STOP,              //停止清洗拖布
    CMD_BASE_RESPONSE,				//基站响应洗拖布
    CMD_BLOW_OVER,
};
#endif