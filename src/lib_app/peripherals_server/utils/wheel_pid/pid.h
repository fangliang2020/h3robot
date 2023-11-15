#ifndef PID_H
#define PID_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include "peripherals_server/base/comhead.h"
typedef struct 
{	
	int16_t speed;		//期望速度，是PID参数的匹配值
	float kp;
	float ki;
	float kd;
	int iLimit;		//积分限幅
}PidParameter;
extern PidParameter pidValue[4];
extern PidParameter pidValue_last[];
typedef struct
{
	PidParameter* pidParameter;	//模糊PID参数
	int currentError;						//当前误差
	int lastError;							//上次误差
	int preError; //上上次误差
	int sumError;								//
	int out;
	int Sumout;
	int16_t Desire;  //设定值
	int16_t Measure; //测量值
}MotorPid;


extern MotorPid rMotorPid;
extern MotorPid lMotorPid;
extern MotorPid fMotorPid;
extern MotorPid mMotorPid;
int Bsp_PidInit(void);
int MotorPidclear(MotorPid *Mpid);
int PidFun_left(Motol_Ctrl *Motor_Item);
int PidFun_right(Motol_Ctrl *Motor_Item);
int PidFun_Mid(Motol_Ctrl *Motor_Item);
int PidFun_Fan(Motol_Ctrl *Motor_Item);
int PidFun_Mop(Motol_Ctrl *Motor_Item);
#endif

