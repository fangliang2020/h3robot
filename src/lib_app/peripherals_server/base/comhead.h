#ifndef  COMHEAD_H
#define  COMHEAD_H
#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <math.h>
#include <thread>
#include <fcntl.h>
#include <vector>
#include <variant>
#include <map>
#include "poll.h"
#include <functional>
#include <mutex>
#include <list>
#include <sys/ioctl.h>
#include "sys/select.h"
#include <stdint.h> 
#include <linux/input.h>
#include <condition_variable>



struct Sensor_Prot
{
	uint8_t status;					//0是正常态，1是保护态
	uint16_t overcount; 		//超出计数
  uint16_t undercount;		//小于计数
	uint16_t resumecount;   //恢复时间
	uint16_t readValue;			//读取寄存器的值
	float currentValue;			//换算后的值
	float OverValue;	     //保护值
	float UnderValue;			 //恢复值
	uint8_t  is_moving;   //是否处于动作的状态 
	Sensor_Prot()
    : status(0),
      overcount(0),
      undercount(0),
			resumecount(0),
			readValue(0),
			currentValue(0),
			OverValue(0),
			UnderValue(0),
			is_moving(0) {
  }
};  //各个传感器的采集值，保护值，恢复值
struct Motol_Ctrl
{
	uint8_t workstate; //工作状态  对于左右轮电机 0:停止 1:正向 2:反向 3:刹车
	int   setspeed;  //设置速度
	int   readspeed; //读取速度
	Sensor_Prot M_Prot; //电流采集值
	Motol_Ctrl()
    : workstate(0),
      setspeed(0),
      readspeed(0) {
  }
};
struct POWER_ADC
{
	uint8_t  status;				//状态
	uint16_t overcount;  		//超出计数
	uint16_t undercount;		//小于计数
	uint16_t readValue;			//读取寄存器的值
	float currentValue;			//换算后的值
	float OverValue;	//保护值
	float UnderValue;			//欠压
	POWER_ADC()
    : status(0),
      overcount(0),
      undercount(0),
			readValue(0),
			currentValue(0),
			OverValue(0),
			UnderValue(0) {
  }
};

struct IMU_VALID_DATA{
	int gyro_x_adc;
	int gyro_y_adc;
	int gyro_z_adc;
	int accel_x_adc;
	int accel_y_adc;
	int accel_z_adc;
	int temp_adc;

	int gyro_x_offset;
	int gyro_y_offset;
	int gyro_z_offset;
	int accel_x_offset;
	int accel_y_offset;
	int accel_z_offset;
	int temp_offset; 
	IMU_VALID_DATA()
    : gyro_x_adc(0),
      gyro_y_adc(0),
      gyro_z_adc(0),
			accel_x_adc(0),
			accel_y_adc(0),
			accel_z_adc(0),
			temp_adc(0),
			gyro_x_offset(0),
	 		gyro_y_offset(0),
			gyro_z_offset(0),
	 		accel_x_offset(0),
	 		accel_y_offset(0),
			accel_z_offset(0),
	 		temp_offset(0) {
  }
};

struct IMU_REAL{
	float gyro_x_act;
	float gyro_y_act;
	float gyro_z_act;
	float accel_x_act;
	float accel_y_act;
	float accel_z_act;
	float temp_act;
	float course_angle;
	IMU_REAL()
    : gyro_x_act(0),
      gyro_y_act(0),
      gyro_z_act(0),
			accel_x_act(0),
			accel_y_act(0),
			accel_z_act(0),
			temp_act(0) {

		}
};
struct GROUD_DET
{
	uint8_t status;
	uint8_t setcnt; //地检检测计数
	uint16_t readVal; //ADC采集读取值
	uint16_t underVal;// 在多少值以内计数减减
};

#pragma pack(4)
struct ODM_Offset
{  
	float Wheel_L_offset;  
	float Wheel_R_offset;  
	float Wheel_DIS_offset;
};//里程计偏移值
#pragma pack()

struct ODM_Cal
{
	int16_t dcL;
	int16_t dcR;
	double runTime;
	float cal_vx;
	float cal_vy;
	float cal_vw;//里程计反解值
	float  vx;
	float  vy;
	float  vyaw;//v和w
	int leftv;
  int rightv;
	float p_vx;
	float p_vy;
	float p_vyaw;
	ODM_Cal()
    : dcL(0),
      dcR(0),
      cal_vx(0),
			cal_vy(0),
			cal_vw(0),
			vx(0),
			vy(0),
			vyaw(0),
			leftv(0),
  	  rightv(0) {
		}
}; 

struct SPD_DET
{
	uint8_t   DIR;  //方向
	uint8_t   LASTDIR;  //上个方向
	int   cnt;  //在中断里计数
	int   rpm; //转速 定义为1s钟多少转，20ms进中断，根据cnt和方向值，以及码盘上多少齿来算1秒钟的转速
	int   rpm_fil; //滤波后的值
};

enum LEV_E{	
	LEV_STOP=0,		//停止0
	LEV_QUIET,		//安静1
	LEV_ORDINARY,	//普通2
	LEV_STRONG, //强力3	
};

enum UPDOWM{
	UPRAISE=0,
	PUTDOWN,
};

//设置共享的参数
class CommParam{
public:
	CommParam(){};
	static Motol_Ctrl Motorfan;
	static Motol_Ctrl UpdownMotor;
	static Motol_Ctrl MotorsideA;
	static POWER_ADC  VBAT_Prot;
	static POWER_ADC  AdaptIn_Prot;
	static Motol_Ctrl RightWheel;
	static Motol_Ctrl LeftWheel;
	static Motol_Ctrl MotorsideB;
	static Motol_Ctrl MotorCleanPump;
	static Motol_Ctrl Motormid;
	static Motol_Ctrl MotorSewagePump;
	static Motol_Ctrl MotorMop;//SCRAPER_MOTOR
	static  GROUD_DET GND_Sensor_RB;   
	static  GROUD_DET GND_Sensor_RF;  
	static  GROUD_DET GND_Sensor_LB;  
	static  GROUD_DET GND_Sensor_LF; 	
	static IMU_VALID_DATA ImuData;
	static IMU_REAL ImuReal;	
	static std::vector<uint16_t>	vl53_dis;
	static uint8_t vl6180_dis;
	static ODM_Cal gODM_CAL;
	static uint8_t abnormal;
	static uint8_t rug;//超声波地毯识别检测数据
	static std::map<std::string,uint8_t> Gpio_PinD;
	static bool ImuRdflag;
	static uint16_t left_irrev;
	static uint16_t right_irrev;
};

#endif // ! COMMONHEAD_H
  