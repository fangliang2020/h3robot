/***********************************************/
/***********************************************
 * 设备状态和清扫状态宏
 * @author dingjinwen
 * @date   2023/1/5
 * *******************************************/
//#include "MainStates.h"
//#include <vector>
//#include <string>
#ifndef DEVICE_STATES_H
#define DEVICE_STATES_H
#include <iostream>
#include <string>
enum E_MASTER_STATE
{
	ESTATE_INIT = 0,
	ESTATE_IDLE = 1,			//空闲
	ESTATE_CLEAN = 2,			//清扫中
	ESTATE_PAUSE_CLEAN = 3,     //清扫暂停
	ESTATE_BACK_CHARGE = 4,     //回充中
	ESTATE_BACK_WASH = 5,       //回洗中
	ESTATE_OTA = 6,             //ota中
	ESTATE_CHAGRE = 7,          //充电中
	ESTATE_CHAGRE_END = 8,      //充电完成
    ESTATE_SHELF_CLEAN = 9,          //自清洁中
    ESTATE_SHELF_CLEAN_END = 10,      //自清洁完成
    ESTATE_DORMANCY = 11,		//休眠
    ESTATE_ERROR = 12,           //错误中
	ESTATE_CHAGRE_PAUSE = 13,    //回充暂停
	ESTATE_WASH_PAUSE = 14,      //回洗暂停
    ESTATE_DUMP = 15,            //集尘中
    ESTATE_DUMP_END = 16,      //集尘完成
    ESTATE_SHUTDOWM = 17,           //关机
    ESTATE_FACTORY_TEST = 18, //产测模式
    ESTATE_FAST_BUILD_MAP = 19,  //快速建图
    ESTATE_PAUSE_FAST_BUILD_MAP = 20, //暂停建图
    ESTATE_FAST_BUILD_MAP_END = 21,  //快速建图完成
    ESTATE_BACK_CHARGE_ERROR = 22, //回充失败,未找到基座
    ESTATE_BACK_WASH_ERROR = 23, //回洗失败,未找到基座
    ESTATE_LOCATE_ERROR = 24, //定位失败
    ESTATE_DEVICE_TRAPPED = 25, //机器被困
    //ESTATE_SET_NETWORK =26,  //配网中
    //ESTATE_SET_NETWORK_END = 27, //配网结束

};
enum E_FAULT_STATE
{
    FAULT_LOW_BATTERY = 1, //电量不足
    FAULT_ERROR_BATTERY = 2, //电池有异常
    FAULT_ERROR_FAN = 3, //风机有异常
    FAULT_ERROR_EDGE = 4, //请擦拭右侧沿墙传感器
    FAULT_ERROR_BUMP = 5, //请检查防撞栏是否卡住
    FAULT_NO_LEVEL_GROUND = 6, //请将机器放于水平地面启动
    FAULT_ERROR_CLIFF = 7, //请擦拭悬崖传感器并远离危险区域启动
    FAULT_NO_GROUND = 8, //请放回地面地再启动
    FAULT_ERROR_LEFT_SPEED = 9, //请检测左轮是否卡住
    FAULT_ERROR_RIGHT_SPEED = 10, //请检测右轮是否卡住
    FAULT_MOP_STUCK = 11, //请检测拖布升降是否卡住
    FAULT_MAIN_BRUSH_STUCK = 12, //请检测主刷是否卡住
    FAULT_EDGE_BRUSH_STUCK = 13, //请检测边刷是否卡住
    FAULT_LIDAR_STUCK = 14, //请检测雷达是否卡住
    FAULT_LIDAR_COVER = 15, //请检查雷达是否被遮挡，并移到新位置启动
    FAULT_DUST_ERROR = 16, //尘盖未盖好，请检查
    FAULT_BILGE_TANK = 17, //污水箱水满，请检查
    FAULT_CLEAN_TANK = 18, //净水箱缺水，请检查
    FAULT_MAIN_BRUSH_ERROR = 19, //主刷异常
    FAULT_MAIN_WHEEL_ERROR = 20, //主轮异常
    FAULT_EDGE_BRUSH_ERROR = 21, //边刷异常
    FAULT_LIDAR_ERROR = 22, //雷达异常
    FAULT_MOP_ERROR = 23, //拖布抬升异常
    FAULT_BASE_ERROR = 24, //基座故障
    FAULT_DIRT_WATER_FULL = 25, //污水过滤槽水满，请处理


};
struct map_update
{
    int map_id = 1;
    std::string map_info_path;
    std::string map_data_path;
};
//typedef EDeviceState E_MASTER_STATE;
// enum SLAM_STATE
// {
//     SLAM_IDLE = 0,
//     SLAM_RUN = 1,
//     SLAM_PAUSE = 2,
//     SLAM_ERROR = 3,
// };
enum CLEAN_STATE
{
    CLEAN_STATE_NULL = 0,   //未清扫
    CLEAN_STATE_AUTO = 1,   //全局清扫
    CLEAN_STATE_RECT = 2,   //划区清扫
    CLEAN_STATE_LOCATION = 3,   //定点清扫
    CLEAN_STATE_SITU = 4,       //指哪扫哪
	CLEAN_STATE_RESERVATION = 5, //预约清扫

};
//typedef CLEAN_STATE E_CLEAN_STATE;
enum EJobCommand
{
    EJOB_NONE = 0,
    EJOB_GLOBAL_CLEAN = 1, //全屋
    EJOB_LOCAL_CLEAN = 2,  //划区
    EJOB_ZONE_CLEAN = 3,   //定点清扫,房间清扫
    EJOB_POINT_CLEAN = 4,  //指哪扫哪
    EJOB_RESERVE_CLEAN = 5,//预约清扫
    EJOB_BACK_CHARGE = 6,

    EJOB_BACK_WASH = 7,

    EJOB_STOP_CLEAN = 8,
    EJOB_PHOTO_PATH = 9,
    EJOB_OTA_START = 10,
    EJOB_BACK_DUMP = 11,
    EJOB_FAST_BUILD_MAP = 12,
    EJOB_VLOW_BACK_CHARGE = 13,
    EJOB_SHUTDOWM = 14,
    EJOB_RESUME_CLEAN = 15,
    EJOB_PAUSE_CLEAN = 16,
    EJOB_NULL = 17,
};
enum LidarState
{
    LIDAR_STATE_NORMAL = 0,
    LIDAR_STATE_ERROR = 1,

};
//尘盒
enum DustState
{
    DUST_STATE_NORMAL = 0,
    DUST_STATE_ERROR = 1,
};
//超声波材质检测
enum TextureState
{
    TEXT_SMOOTH = 0, //光洁面
    TEXT_BLANKET = 1, //毛毯面
    TEXT_ERROR = 2, //传感器错误
};
//清水泵
enum PurifiedState
{
    PUR_STATE_NORMAL = 0, //正常
    PUR_STATE_OPEN = 1, //开路
    PUR_STATE_OVER = 2, //过流
 
};
enum WheelLeft
{
    WHEEL_LEFT_NORMAL = 0,
    WHEEL_LEFT_OVER = 1,
    WHEEL_LEFT_STOP = 2, //堵转
    WHEEL_LEFT_OPEN = 3,
};
enum WheelRight
{
    WHEEL_RIGHT_NORMAL = 0,
    WHEEL_RIGHT_OVER = 1,
    WHEEL_RIGHT_STOP = 2,
    WHEEL_RIGHT_OPEN = 3,

};
//中扫电机
enum MidmotorState
{
    MID_STATE_NORMAL = 0,
    MID_STATE_OVER = 1,
    MID_STATE_STOP = 2,
    MID_STATE_OPEN = 3,

};
//风机
enum FanmotorState
{
    FAN_STATE_NORMAL = 0,
    FAN_STATE_OVER = 1,
    FAN_STATE_STOP = 2,
    FAN_STATE_OPEN = 3,

};
//左边扫电机
enum SideLeftState
{
    SIDE_LEFT_START_NORMAL = 0,
    SIDE_LEFT_STATE_OVER = 1,
    SIDE_LEFT_STATE_STOP = 2,
    SIDE_LEFT_STATE_OPEN = 3,

};
enum SideRightState
{
    SIDE_RIGHT_STATE_NORMAL = 0,
    SIDE_RIGHT_STATE_OVER = 1,
    SIDE_RIGHT_STATE_STOP = 2,
    SIDE_RIGHT_STATE_OPEN = 3,

};
//拖布电机
enum MopmotorState
{
    MOP_STATE_NORMAL = 0,
    MOP_STATE_OVER = 1,
    MOP_STATE_STOP = 2,
    MOP_STATE_OPEN = 3,
};
//电池状态
enum BatteryState
{
    BATTERY_STATE_NORMAL = 0,
    BATTY_STATE_LOW = 1, //低压
    BATTY_STATE_HIGHT = 2, //过压
    BATTY_STATE_CHARGE_PASS = 3, //充电超时
    BATTY_STATE_HIGHT_TEMP = 4, //高温
    BATTT_STATE_UNABLE_CHARGE = 5, //不能够充电

};
//沿边传感器
enum EdgeSensorState
{
    EDGE_STATE_NORMAL = 0,
    EDGE_STATE_COVER = 1, //灰尘覆盖
    EDGE_STATE_ERROR = 2,

};
//前TOF传感器
enum AvoidSensorState
{
    AVOID_STATE_NORMAL = 0,
    AVOID_STATE_COVER = 1,
    AVOID_STATE_ERROR = 2,


};
#endif