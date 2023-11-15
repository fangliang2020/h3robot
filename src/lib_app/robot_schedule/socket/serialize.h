#ifndef APP_SERIALIZE_H
#define APP_SERIALIZE_H
//#include "Clean/apptypes.h""
//#include "../global/types.h"
//#include "../global/tools.h"
//#include "../global/debug.h"
//#include <iostream>
#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <thread>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "robot_schedule/clean/apptypes.hpp"


void serialize_short(U16 data,char* out,int *len);			//适用于2字节序列化
void serialize_int(U32 data,char* out,int *len);			//适用于4字节序列化,注意long和double不能手动序列化

void serialize_imu_data(IMU_DATA *data,char *out,int *len);
void serialize_odom_data(ODOM_DATA *data,char *out,int *len);
void serialize_point_data(Point *data,char *out,int *len);
void serialize_block_data(BLOCK_DATA *data,char *out,int *len);	//单个BLOCK_DATA序列化
void serialize_blocks_data(BLOCK_DATA *data,int blen,char *out,int *len); //多个BLOCK_DATA序列化
void serialize_pose_data(ROBOT_POSE *data,char *out,int *len);
void serialize_zone_data(ROBOT_POSE *data0,Point *data1,char *out,int *len);

void serialize_socket_map_data(SOCKET_MAP_DATA *data,char *out,int *len);
void serialize_socket_pose_data(SOCKET_POSE_DATA *data,char *out,int *len);
void serialize_socket_zone_data(SOCKET_ZONE_DATA *data,char *out,int *len);
void serialize_socket_imu_data(SOCKET_IMU_DATA *data,char *out,int *len);
void serialize_socket_odom_data(SOCKET_ODOM_DATA *data,char *out,int *len);
//extern int gAngle; //全局陀螺仪角度,范围[-180,+180],-180为X负,+180为X正;+90为Y正;-90为Y负
//extern Point * coordinates;
#endif

