#ifndef _APP_NAVIGATE_H
#define _APP_NAVIGATE_H
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include "thread_pool.h"
#include "msgqueue.h"
#include "serialize.h"
#include "robot_schedule/clean/cmd_interface.hpp"
#include "robot_schedule/clean/fsm.hpp"

//typedef char U8;
#define UNIX_DOMAIN_NAVI_IMU		"/tmp/navigate_imu.domain"		//IMU高频数据发送
#define UNIX_DOMAIN_NAVI_ODOM		"/tmp/navigate_odom.domain"		//ODOM高频数据发送
#define UNIX_DOMAIN_NAVI_POSE		"/tmp/navigate_pose.domain"		//POSE高频数据发送
#define UNIX_DOMAIN_NAVI_LOW		"/tmp/navigate_low.domain"	//接收端(包含低频数据发送)
#define UNIX_DOMAIN_NAVI_DIST		"/tmp/navigate_dist.domain"	//DIST高频数据发送
#define DEFAULT_BUFFER_SIZE 1024

//负责发送运控数据给导航-socket，并接收导航数据给运控-回调函数
#define MAP_DATA_HEAD_CHAR1		0xaa
#define MAP_DATA_HEAD_CHAR2		0xbb	//栅格信息包头
#define POSE_DATA_HEAD_CHAR1	0xcc
#define POSE_DATA_HEAD_CHAR2	0xdd	//机器位姿包头
#define POINT_DATA_HEAD_CHAR1	0xee
#define POINT_DATA_HEAD_CHAR2	0xff	//4_4分区包头
#define IMU_DATA_HEAD_CHAR1		0xab
#define IMU_DATA_HEAD_CHAR2		0xcd	//IMU包头
#define ODOM_DATA_HEAD_CHAR1	0xbc
#define ODOM_DATA_HEAD_CHAR2	0xde	//ODOM包头
#define CMD_HEAD_CHAR1			0x12
#define CMD_HEAD_CHAR2			0x34	//命令cmd包头
#define LINE_HEAD_CHAR1			0x56	
#define LINE_HEAD_CHAR2			0x78	//直线角度包头
#define DIST_HEAD_CHAR1			0x9a	
#define DIST_HEAD_CHAR2			0xbc	//障碍物距离(高频)
#define PATH_HEAD_CHAR1			0x9b	
#define PATH_HEAD_CHAR2			0xac	//指定点到点的路径包头
#define TOF_HEAD_CHAR1			0x3a
#define TOF_HEAD_CHAR2			0x3b

//头和校验长度定义
#define HEAD_LEN		2
#define CHECK_LEN		2
//结构体长度定义
#define POINT_LEN		sizeof(Point)		//8
#define IMU_LEN			sizeof(IMU_DATA)	//48
#define ODOM_LEN		sizeof(ODOM_DATA)	//8
#define BLOCK_LEN		sizeof(BLOCK_DATA)	//9
#define POSE_LEN		sizeof(ROBOT_POSE)	//12
#define ZONE_LEN		sizeof(ZONE_POSE)
#define TIMESTAMP_LEN	sizeof(long)		//8
#define DIST_LEN		sizeof(int)			//4
//定长数据包长度定义
#define IMU_PACKAGE_LEN				HEAD_LEN + IMU_LEN  + TIMESTAMP_LEN + CHECK_LEN		//60
#define ODOM_PACKAGE_LEN			HEAD_LEN + ODOM_LEN + TIMESTAMP_LEN + CHECK_LEN		//20	
#define POSE_PACKAGE_LEN			HEAD_LEN + POSE_LEN + CHECK_LEN						//16
#define ZONE_PACKAGE_LEN			HEAD_LEN + ZONE_LEN + 2*POINT_LEN + CHECK_LEN 		//32
#define CMD_PACKAGE_LEN				HEAD_LEN + 1 + CHECK_LEN  // 5
#define LINE_PACKAGE_LEN			8
#define DIST_PACKAGE_LEN			HEAD_LEN + DIST_LEN + CHECK_LEN  //8
#define PATH_PACKAGE_LEN			HEAD_LEN + 2*POINT_LEN + CHECK_LEN  //20


//extern int gAngle; //全局陀螺仪角度,范围[-180,+180],-180为X负,+180为X正;+90为Y正;-90为Y负
//extern Point * coordinates;
//int gAngle; //全局陀螺仪角度,范围[-180,+180],-180为X负,+180为X正;+90为Y正;-90为Y负
//Point * coordinates;
extern bool map_receive_finish;
void navigate_init();
//void *navigateloop(void *agv);
void navigateloop();
void odom_callback(int speed_left,int speed_right);
void imu_callback(double angleX,double angleY,double angleZ,double accX,double accY,double accZ);
//控制命令
void send_cmd(U8 cmd);
void followwall_maps_upload(BLOCK_DATA *data,int len);//沿边地图
void get_route(Point start,Point end);
#endif