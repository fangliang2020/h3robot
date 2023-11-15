#ifndef _APP_ZMODE_H
#define _APP_ZMODE_H

//#include "../global/types.h"
//#include "../global/tools.h"
//#include "../global/debug.h"
#include "apptypes.hpp"
#include "fsm.hpp"
#include "map.hpp"
#include "dwa.hpp"
#include "config.hpp"
#include "tools.hpp"
#include "robot_schedule/socket/navigate.h"
/**
*标准弓扫模式的定义：
* @note:本系统均采用[直角坐标系],与之前惯导、视觉机器不一样[直角坐标系逆时针转90度]。
**/

//子状态-弓扫状态枚举
typedef enum zmode_e
{
	ZMODE_LONG_EDGE=0,		   		//弓字长边		  
	ZMODE_SHORT_EDGE,		   		//弓字短边
	ZMODE_BACK,              		//硬碰撞后,后退
	ZMODE_FOLLOW_BACK,
	ZMODE_BACKA,					//双硬碰撞触发后退后切换
	ZMODE_LONG_TO_SHORT_TURN,		//长边走完后转向到短边,转弯前需stop(100ms)
	ZMODE_SHORT_TO_LONG_TURN,		//短边走完后转向到长边
	ZMODE_DROP,			   			//跌落发生
	ZMODE_DROP_TURN,				//遇到跌落的转向
	ZMODE_ERROR,					//故障发生    
//	ZMODE_LONG_EDGE_H_TURN,			//长边碰障碍,原地转90度
	ZMODE_FOLLOW,					//沿边状态10
//	ZMODE_S_TURN,					//短边中软触发(大障碍物),转弯
	ZMODE_H_TURN,					//短边+长边硬碰障碍
	ZMODE_FOLLOW_TURN,
	ZMODE_MAP_SEARCH,				//地图搜索未清扫的区域
	ZMODE_GET_PATH,                 //请求slam返回到达目标点的路径
	ZMODE_TO_TARGET_POINT,			//导航到目标地点
	ZMODE_GO,						//
	ZMODE_SEARCH_NEXT,				//到达下一个分区弓扫点(沿墙点)	
	ZMODE_BORDER_TURN,				//边界进行转弯
	ZMODE_IMPACT_SHIFT,
	ZMODE_RESUME_TO_LONG_TURN,
} ZMODE_E;

//弓扫状态机循环	
typedef struct zmode_state
{
	ZMODE_E cur_state;
	void (*ctrlfunc)(void);
	void (*ctrlinit)(void);
} ZMODE_STATE;
//各个状态里面的事件处理及内部状态切换
typedef struct zmode_event
{
	ZMODE_E cur_state;
	bool (*event)(void);
	ZMODE_E next_state;
} ZMODE_EVENT;

typedef enum zmode_type{
	left_mode=0,	//左弓扫
	right_mode		//右弓扫
}ZMODE_TYPE;
typedef enum clean_direct{	//清扫方向
	positive_x = 0,			//直角坐标系下,X正方向
	positive_y,				//Y正方向
	negative_x,				//X负
	negative_y				//Y负
}CLEAN_DIRECT;
typedef enum follow_type{	//沿墙类型
	wall_left=0,			//左沿墙(墙在机器的左边)
	wall_right				//右沿墙(墙在机器的右边)
}FOLLOW_TYPE;
typedef enum line_switched_state{
	unswitched=0,
	switched
}LINE_SWITCHED_STATE;


#define LEFT_IMPACT		0		//碰撞位置-左硬碰撞
#define RIGHT_IMPACT	1		//碰撞位置-右硬碰撞
#define BOTH_IMPACT		2		//左右碰撞均发生
#define SOFT_IMPACT		3		//中软碰撞

#define PRELINE_NOT_FIND		0	//前条线暂未找到
#define PRELINE_FIND			1	//已找到

#define IMPACT_BACK		0		//因碰撞而后退
#define CARPET_BACK		1		//因地毯而后退
#define SOFT_BACK       2       //软碰撞 无需后退

#define LONG_EDGE		0		//当前所处位置-长边
#define	SHORT_EDGE		1		//当前所处位置-短边

#define LONG_EDGE_START		0	//长边开始(短边结束)
#define LONG_EDGE_END		1	//长边结束(短边开始)

#define ANGLE_SELF		0		//自身角度微调目标角度
#define ANGLE_TARGET	1		//转向目标角度
#define TURN_ANGLE		3000

// z_mode模式下变量定义
typedef struct zmode_params
{
	U8 		restart;					//重新进入新的弓扫,防止按暂停键后重复初始化
	U8 		cur_zmode;					//当前的弓扫模式,左弓扫、右弓扫
	U8		cur_edge;					//当前所处位置:长边-LONG_EDGE,或短边-SHORT_EDGE
	U8   	cur_cleandirect;			//当前清扫方向(机器运动方向),取值0:X正;1:Y正;2:X负;3:Y负
	U8   	cur_followtype;				//当前沿墙方向,取值0:左沿墙(墙在机器的左边),1:右沿墙
	U8		followdirect;				//沿边清扫的方向,用于局部碰撞后沿边方向
	U8 		edge_state;					//长边|短边状态:长边开始-结束，短边开始-结束	
	int 	fix_angle;					//校准角度	
	int 	target_angle;				//目标转向角度
	int 	target_pose;				//PID距离目标值,X值或Y值
	
	U8 		impact_side;				//碰撞位置,左碰撞-LEFT_IMPACT,右碰撞-RIGHT_IMPACT,BOTH_IMPACT,中软SOFT_IMPACT
	int 	now_line_x;					//机器x坐标的行距倍数,取值范围可正可负
	int     next_line;
	int 	prev_line;
	U8   	line_switched;				//弓扫线路是否已经切换,0:未更新;1:更新了

	long  	itime;						//时间戳,动作开始时的时间,通常用于计算一套动作的时间差		
	U8 		preline_find;				//走长边,碰到障碍物、沿障碍物、最后找到了前线的标记，0-未找到,1-找到
	
	Point  	*coordinates;				//记录当前坐标
	Point	zmode_start_pose;			//弓扫起始点
	Point	zmode_end_pose;				//弓扫起始点的对角点坐标,zmode_start_pose和zmode_end_pose构成弓扫边界
	U8 		drop_type;					//跌落类型：
	U8  	back_type;					//后退类型,0:碰撞后退;1:地毯后退
	BLOCK_DATA	*block;					//地图块数据
	BLOCK_DATA  *pblock;				//指向地图块尾部
	int		block_len;					//地图块数据个数
	int 	target_path_index;			//去目标点的数组索引
	int		update_line_flag;			//更新线
	int 	search_area_flag;			//搜索出空白区域没有1:有 0 :没有
	int		suppmentary_direct;			//补扫的方向
	int		long_short_state;			//1:长边状态,0:短边状态
	int		pre_line;					//之前那条线
	int		border_short_edge;					//0:代表走短边的形式 1：代表一个轮不转一个转的形式
	int 	suppmentary_flag ;			//判断是否在补扫状态
	int 	left;
	int 	right;
	int 	down;
	int 	top;
}ZMODE_PARAMS;

extern ZMODE_E gzmode_state;
extern U8 zmode_follow_direct;
void zmode_init();
void zmode_cmd_download(int cmd);
//在fsm里循环调用
void zmode_perform();
void zmode_var_reset();
void zmode_set_border(int left_x,int right_x,int up_y,int down_y);
void zmode_border_reset();
void zmode_callback(int startX,int startY,int endX,int endY);
void api_set_wheel_pid(int32_t left, int32_t right, int16_t angle,int en);
#endif	//EOF _APP_ZMODE_H
