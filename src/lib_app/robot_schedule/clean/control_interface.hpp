#ifndef _CONTROL_INTERFACE_H
#define _CONTROL_INTERFACE_H
#include "fsm.hpp"
#include "robot_schedule/socket/serialize.h"
#include "debug.hpp"
#include <iostream>
#include <vector>
#include <string>
#include "robot_schedule/schedule/DbdataBase.h"
struct Room
{
	int vertexs_value[4][2];
	std::string bedroom_name;
	int room_id;
};



typedef enum room_name
{
	BATH_ROOM,                     //卫生间
	LIVING_ROOM,                  //客厅
	DINNING_ROOM,                  //餐厅
}ROOM_NAME;

//中断工作
void interrupt_work();
//暂停工作
void pause_work();
//恢复工作
void resume_work();
//全屋清扫
uint8_t house_clean(char from_base,char map_id,char cotinue_clean);
//指定房间清扫,参数1：房间 参数2：当前工作状态0刚从基站出仓 1正在执行清扫任务
uint8_t room_clean(ROOM_NAME room,char from_base);
//指定区域清扫，参数1：左下角坐标 参数2：右上角坐标00
uint8_t area_clean(Point bottomLeftCorner ,Point topRightCorner);
void setVWtoRobot(int16_t v,int16_t w);

void set_forbidden_zone(std::vector<Room> &forbidden_info);

#endif