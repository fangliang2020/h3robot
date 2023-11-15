#ifndef _APP_TYPES_H
#define _APP_TYPES_H
//#include "../global/types.h"
//#include "../global/debug.h"
//#include "../global/tools.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
//#include <vector>
#include "debug.hpp"
//using namespace std;
typedef unsigned char U8;
typedef unsigned short int U16;
typedef unsigned int U32;
typedef signed char S8;
typedef signed short S16;
typedef signed int S32;


#define PI		3.1415926
#define ON    1
#define OFF   0

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//////////////////////////////////应用结构定义/////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//顶层状态枚举
typedef enum topmode_e
{
	TOPMODE_CLEAN = 0, 		//等待开机按键或继续清扫0
	TOPMODE_QUICK_MAPPING,
	TOPMODE_FOLLOWWALL,		//沿墙模式
	TOPMODE_ZMODE,			//弓扫模式
	TOPMODE_STANDBY,		//待机模式6
	TOPMODE_PAUSE,		    //暂停模式
	TOPMODE_FINISH,
} TOPMODE_E;

typedef enum cmd_e
{
	CMD_BASE_DEFAULT,
	CMD_BASE_CALLBACK,	//基站发出召回-0
	CMD_BASE_OUT,			//基站发出出仓-1
	CMD_BASE_CLEAN,			//基站发出清扫-2
	//CMD_BASE_PAUSE,			//基站发出暂停-3
	// CMD_MAIN_CLEAN,			//主机按键-4
	// CMD_MAIN_PAUSE,			//主机按键-5
	// CMD_HAND_CHARGE,		//手动充电-6
}CMD_E;

//slam发送给主控的消息
enum {
    CMD_RELOCATION_SUCCESS,
    CMD_RELOCATION_FAIL,
    CMD_FRONT_OBSTACLE,//直接转向
    CMD_FRONT_SAFE,
    CMD_LOAD_MAP_SUCCESS,
    CMD_LOAD_MAP_FAILURE,
	CMD_PATH_FAILURE,//6
	CMD_FRONT_BORDER,//减速
	CMD_STRLINE_SUCCESS,
	CMD_OUICK_MAPPING_DONE,
};

//主控发送给slam的消息
enum {
 CMD_GET_FOLLOWWALL_ZONE,
 CMD_GET_ZMODE_MAP,
 CMD_GET_STRIGHT_LINE,
 CMD_LOAD_AREA_MAP,
 CMD_LOAD_GLOBAL_MAP, //4
 CMD_HANG_IN_THE_AIR, //5
 CMD_NOT_HANG_THE_AIR,  //6
 CMD_NEED_RELOCATE, //7
 CMD_REQUEST_QUICK_MAPPING, //8
 CMD_QUICK_MAPPING_OBSTACLE, //9
 CMD_RELOCATION_NEW_ZONE,//10重定位成功但是不在分区内
 CMD_SLEEP_SLAM, //11,
 CMD_AWAKE_SLAM,  //12
 CMD_FINISH_RECHARGE,  //13
 CMD_CLEAN_BATH_ROOM, //16清扫卫生间
 CMD_CLEAN_LIVING_ROOM, //17清扫客厅
 CMD_CLEAN_DINNING_ROOM, //18清扫餐厅
};

#define MACHINE_WIDTH	360				//机身直径360mm
#define HALF_MACHINE_WIDTH MACHINE_WIDTH/2	//半个机身
#define LINE_SPACE		MACHINE_WIDTH/2		//默认清扫间距为半个机身(180mm)
#define CIRCLE_LINE 140 //一个轮转，一个轮不转的行距
//机器位置及基站命令
typedef enum robot_location_e
{
	IN_BASE=0,			//在基站内0
	NOT_IN_BASE,		//不在基站内1
}ROBOT_LOCATION_E;



#define ERROR_ANGLE 			600 	//默认误差为4度进行微调
#define LINE_TURN_ANGLE			300	//开机找直线转角
#define DEFAULT_ANGLE_ERROR		500		//转直角默认误差角度0.5度,由于时基原因,不一定能到达0度
#define DECLI_ANGLE				1000	//偏角declinationn
#define CIRCLE_TARGE_DIFF_ANGLE	3000	//走弧形目标角度为微调角度加减CIRCLE_TARGE_DIFF_ANGLE

//电池电量百分比

//触发硬碰撞
#define HardCollisionL (GetData().l == 1)	//1触发,0不触发
#define HardCollisionR (GetData().r == 1)
#define HardCollisionC (GetData().l == 1 && GetData().r == 1)
//未触发硬碰撞
#define HardCollisionCX (GetData().l == 0 && GetData().r == 0)
/*
//左沿墙、右沿墙数据
#define LWallData 		(GetData().v_wall_L)
#define RWallData 		(dwi_vol.v_wall_R)
#define LWallData_Min 	(dwi_vol.basic_noise_L)
#define RWallData_Min 	(dwi_vol.basic_noise_R)

//软碰数据
#define CImpaData 		(dwi_vol.v_wall_F)  // 前挡中软碰数据


#define LImpaData 		(dwi_vol.v_wall_LF) // 前挡左软碰数据
#define RImpaData 		(dwi_vol.v_wall_RF) // 前挡右软碰数据

//左left墙检(近near、中middle、远far、未X)
#define SoftCollisionLN (dwi_vol.b_wall_L == 0)
#define SoftCollisionLM (dwi_vol.b_wall_L == 1)
#define SoftCollisionLF (dwi_vol.b_wall_L == 2)

//左前left-front墙检(近、中、远、未)
#define SoftCollisionLFN (dwi_vol.b_wall_LF == 0)
#define SoftCollisionLFM (dwi_vol.b_wall_LF == 1)
#define SoftCollisionLFF (dwi_vol.b_wall_LF == 2)
//前front墙检(近、中、远、未)

// #define SoftCollisionFN (dwi_vol.b_wall_F == 0)
// #define SoftCollisionFM (dwi_vol.b_wall_F == 1)
// #define SoftCollisionFF (dwi_vol.b_wall_F == 2)

//右前right-front墙检(近、中、远、未)
#define SoftCollisionRFN (dwi_vol.b_wall_RF == 0)
#define SoftCollisionRFM (dwi_vol.b_wall_RF == 1)
#define SoftCollisionRFF (dwi_vol.b_wall_RF == 2)
//右right墙检(近、中、远、未)
#define SoftCollisionRN (dwi_vol.b_wall_R == 0)
#define SoftCollisionRM (dwi_vol.b_wall_R == 1)
#define SoftCollisionRF (dwi_vol.b_wall_R == 2)


//墙检取值配置
#define OOT_B_IR_L (dwi_vol.b_wall_L)
#define OOT_B_IR_LF (dwi_vol.b_wall_LF)
#define OOT_B_IR_F (dwi_vol.b_wall_F)
#define OOT_B_IR_RF (dwi_vol.b_wall_RF)
#define OOT_B_IR_R (dwi_vol.b_wall_R)
#define OOT_B_DUMP_L (dwi_vol.b_dump_L)
#define OOT_B_DUMP_R (dwi_vol.b_dump_R)

//地检取值配置
#define OOT_B_GND_L (dgi_vol.b_drop_L == 1)
#define OOT_B_GND_R (dgi_vol.b_drop_R == 1)
#define OOT_B_GND_LF (dgi_vol.b_drop_LF == 1)
#define OOT_B_GND_RF (dgi_vol.b_drop_RF)
#define OOT_B_GND_LB (dgi_vol.b_drop_LB)
#define OOT_B_GND_RB (dgi_vol.b_drop_RB)
*/

#pragma pack (1)		//结构体设置1字节对齐
//坐标结构体
typedef struct Point
{
    int x;	//保留2位小数,乘以100,比如12.33,x=1233
    int y;
} Point;
//vector<Point> Astar_point;
typedef struct Rectangle
{
    struct Point lb;	
    struct Point rb;
	struct Point rt;
	struct Point lt;
} Rectangle;

//栅格类型
enum block_type{
	UNSWEEPED=0,	//未清扫栅格
	OBSTACLE,		//障碍物栅格
	FOLLOW,			//沿墙走过的路径
	SWEEPED,		//已清扫栅格
	CHARGESEAT,	  	//充电座栅格
	DROP,			//跌落栅格
	FORBIDDEN,
	OBSTACLE_SEND,
};
//栅格信息
typedef struct block_data{
	Point 	point;	//栅格中心点XY坐标(单位:mm)
	U8		type;	//栅格类型,block_type
}BLOCK_DATA;	

//机器位姿
typedef struct robot_pose{
	Point point;	//机器坐标
	int	yaw;		//航向角,//保留2位小数,乘以100,比如12.33,x=1233
	Point robot_device;
	int robot_angle;
}ROBOT_POSE;
//////////////////////////////////////////////////////////////
//机器位姿
typedef struct zone_pose{
	Point point;	//机器坐标
	int	yaw;		//航向角,//保留2位小数,乘以100,比如12.33,x=1233
}ZONE_POSE;
//栅格信息,不定长,数据方向:导航<->运控
typedef struct socket_map_data{		//例如：AABB 900 ....... ccdd
	U16 	head;		//包头,aabb-栅格数据-地图->运控,eeff-运控->地图
	int		length;	//数据长度,实际数据长度,是block_data的整数倍
	BLOCK_DATA *data;	//实际数据	
	U16	check;	//校验码,CRC16
}SOCKET_MAP_DATA;
//机器位姿信息,定长,数据方向:导航->运控
typedef struct socket_pose_data{		//例如：AABB 900 ....... ccdd
	U16 	head;		//包头abcd
	ROBOT_POSE data;	//实际数据,一次发一个数据,长度为sizeof(ROBOT_POSE)	
	U16	check;	//校验码,CRC16,从头到数据结束计算
}SOCKET_POSE_DATA;
//2_2分区,定长,数据方向:导航->运控
typedef struct socket_zone_data{
	U16 	head;		//长度为sizeof(initPose)	 + sizeof(rect)
	ROBOT_POSE initPose;//起始点位姿,坐标+运动方向
	Point rect[2]; //正方型对角点
	U16	check;
}SOCKET_ZONE_DATA;
//////////////////////////////////////////////////////////////
typedef struct imu_data{
	double vel_angle_x;			//角速度
	double vel_angle_y;
	double vel_angle_z;
	double acc_x;				//加速度
	double acc_y;
	double acc_z;
}IMU_DATA;
//imu数据,定长,数据方向:运控->导航
typedef struct socket_imu_data{
	U16 	head;		//包头,abab-运控->地图
	IMU_DATA data;		//实际数据	
	long timestamp;		//时间戳
	U16	check;			//校验码,CRC16
}SOCKET_IMU_DATA;
typedef struct odom_data{	//轮速和码盘数据
	int speed_left;		//轮速,保留2位小数,乘以100,比如12.33,x=1233
	int speed_right;	//轮速
}ODOM_DATA;
//里程计数据,定长,数据方向:运控->导航
typedef struct socket_odom_data{
	U16 	head;	
	ODOM_DATA data;
	long timestamp;		//时间戳
	U16	check;
}SOCKET_ODOM_DATA;

///////////////////////////////////////////////////////////
//所有请求数据命令,数据方向:运控<->导航
typedef struct socket_cmd{
	U16 	head;
	U8		cmd;	//0-获取弓扫局部地图数据;
	U16		check;
}SOCKET_CMD;

#pragma pack ()




#endif
















