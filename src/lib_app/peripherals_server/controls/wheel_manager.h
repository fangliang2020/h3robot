#ifndef _WHEEL_MANAGER_H_
#define _WHEEL_MANAGER_H_
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
#include <math.h>
#include "peripherals_server/base/base_module.h"
#include "peripherals_server/base/comhead.h"
#include "peripherals_server/utils/wheel_pid/pid.h"

#define DEVLWHEEL_PATH "/dev/left-motor-ctrl"
#define DEVRWHEEL_PATH "/dev/right-motor-ctrl"

// #define WHEEL_DIAMETER              0.032f   // 轮子半径
// #define WHEEL_DISTANCE              0.27f    // 轮子间距
// #define REDUCTION_RATIO             65.6f // 减速比
// #define ENCODE_NUMS                 30.0f   // 电机编码器线数
#define M_PI_		3.14159265358979323846f


namespace peripherals{

class WheelManager : public BaseModule,
                     public chml::MessageHandler,
                     public CommParam {
public:
    explicit WheelManager(PerServer *per_server_ptr);
    virtual ~WheelManager();
    // static WheelManagerPtr &GetInstance();
    int Start();
    int Stop();
    int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
    int SetMoveCtrl(Json::Value &jreq, Json::Value &jret);
    int odm_calibrate(Json::Value &jreq, Json::Value &jret);
    void odometry_init(int left_encoder, int right_encoder);
    void odometry_calc(ODM_Cal* Calodmdata);//根据码盘反解出v和w,上报的时候才计算
    void wheel_spd_calc(ODM_Cal* MoveVMdata);//根据v和w解算出setspeed，有下发v和w的时候才计算
    void wheel_tick_ctrl();//根据set和read speed,pid求解出具体的PWM脉宽，定时器20ms 计算一次
    void CodeWheel_Count();//码盘计数,定时器20ms 计算一次
 //   void odometry_calc_velocity(int* leftvel,int* rightvel);
    void Timer20ms();
public:
    void Wheel_Ctrl_Left(int pulse,uint8_t dir,uint8_t state);
    void Wheel_Ctrl_Right(int pulse,uint8_t dir,uint8_t state);//下发pwm
private:
    // static WheelManagerPtr instance_;
     int lwheel_fd; 
     int rwheel_fd;
private:
    ODM_Offset gODM_SAVE;
	SPD_DET L_SPD;//左轮速度计算
	SPD_DET R_SPD;//右轮速度计算   
private:
    chml::EventService::Ptr Event_Service_;
    void OnMessage(chml::Message* msg);
    chml::DpClient::Ptr dp_client_;
};
    
}
#endif
