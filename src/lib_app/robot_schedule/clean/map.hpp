/*
 * @描述: 
 * @版本: 
 * @作者: 
 * @Date: 2021-12-10 10:58:12
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-03-07 17:27:01
 */
#ifndef _APP_MAP_H
#define _APP_MAP_H
//#include "../global/types.h"
//#include "../global/tools.h"
//#include "../global/debug.h"

#include "apptypes.hpp"
#include "math.h"
#include "followwall.hpp"
#define print_map_count  70 
#define BLOCK_WIDTH 50 						//一个栅格的大小50mm
#define HOUSE_WIDTH	40000	 				//房屋大小40米(40000mm)
#define COL		HOUSE_WIDTH/BLOCK_WIDTH		//行列数,HOUSE_WIDTH/BLOCK_WIDTH=800个
#define ROW		HOUSE_WIDTH/BLOCK_WIDTH
#define COORD_FACTOR		COL/2 //坐标的增量 ??
#define BLOCK_SQUARE (BLOCK_WIDTH * BLOCK_WIDTH * 0.001 * 0.001) //一个方格的面积
extern BLOCK_DATA GMAP[COL][ROW];
extern Point center_hard_impact[1];
extern Point left_front_soft_impact_point[1];
extern Point right_front_soft_impact_point[1];
//墙检所有传感器的值,单位信号强度,值越大,信号越强
typedef struct dwi_res_group
{
  int b_dump_L;       //左前档触发状态：0未触发/1触发
  int b_dump_R;       //右前档触发状态：0未触发/1触发
  int b_wall_L;       //左墙检触状态	
  int b_wall_R;       //右墙检触状态		
  int b_wall_LF;      //左前墙检触状态
  int b_wall_RF;      //右前墙检触状态	
  int b_wall_F;       //前墙检触状态
	
  int v_wall_L;      //左墙检检测值
  int v_wall_R;      //右墙检检测值
  int v_wall_LF;     //左前墙检检测值
  int v_wall_RF;     //右前墙检检测值
  int v_wall_F;      //前侧墙检检测值
  int basic_noise_L; //左墙检基础噪声
  int basic_noise_R; //右墙检基础噪声
  int turn_flag;	//是不是在旋转
} dwi_res_group;

typedef struct map_params
{
	int map_path_index;
	int upMax;//上边界
	int downMax;//下边界
	int leftMax;//左边界
	int rightMax;//右边界
	double cleaned_square;//清扫面积
	Point *coordinates;//机器的当前坐标
	int suppmentary_border;//补扫判断的界限
	int map_suppmentary_clean ;//当前处于补扫状态
	int send_time;
}MAP_PARAMS;
typedef struct Open_list open_list, * pOpen_list;
typedef struct Node{
	pOpen_list father;//当前节点的父亲节点
	float G; //欧拉距离:(x1-x2)²+(y1-y2)²可以向八个方向走
	float H; //曼哈顿距离|x1-x2|+|y1-y2|上下左右走
	float F; //欧拉距离 加 曼哈顿距离
	Point point;//机器当前机器的坐标
}Node, *pNode ;
typedef struct Open_list
{
	struct Open_list *next;
	struct Node node;
}Open_list,*pOpen_list;

typedef struct Search_list
{
	struct Search_list *next;
	Point point;
}Search_list,* pSearch_list;
typedef struct Search_list search_list,* pSearch_list;


void map_init();
double get_clean_area();
void map_store(Point center,int angle,dwi_res_group dwires,TOPMODE_E mode);
int map_judge_area_can_go(Point point);//获取坐标点
/**
*	@brief 地图存储
*	@param	任意坐标点
*	@type	栅格类型
*/
void map_set_point_info(Point point,int  type);
void map_set_obstacle(Point center,int angle,int impact_type);
//获取起点到终点的距离
float get_distance(Point start, Point end);
//打印地图
void print_map();
//获取未清扫路径
void map_get_uncleaned_area_path(Point *path,int *path_count);
//搜索未清扫区域初始化
void map_search_init(Point now,Point end);
//搜索未清扫的区域
int map_search_blank_area(Point *unsweeped_point);
//获取清扫的方向
int map_get_clean_direct(Point point);
//判断当前位置是否已经清扫
int map_position_is_cleaned(Point point,int angle, int mode);

void map_zone_init(Point start,Point end);
//5.按路径走

int map_suppmentary_prevline(int angle, int mode);

void map_zone_clean(Point start,Point end);
bool map_judge_border(Point temp );

void* sendmap_tick(void* arg);
bool map_follow_wall_border(int angle);
bool map_follow_obstacle(int angle);

void make_map_data(int x,int y,int attr,char* out);
bool map_get_area_is_border(Point point);
int map_get_point_info(Point point);

void uart_test();
#endif