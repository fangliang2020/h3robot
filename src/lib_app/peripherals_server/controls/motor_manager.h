#ifndef _MOTOR_MANAGER_H_
#define _MOTOR_MANAGER_H_
#include "peripherals_server/base/base_module.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/event/messagehandler.h"
#include "peripherals_server/base/comhead.h"
#include "peripherals_server/utils/wheel_pid/pid.h"


#define DEVFAN_PATH "/dev/fan-motor-ctrl"
#define DEVMOP_PATH "/dev/mop-motor-ctrl"
#define DEVMID_PATH "/dev/mid-pwm"
#define DEVSIDE_PATH "/dev/side-pwm"
#define ULTRA_PATH "/dev/i2c-3"

#define FAN_PWR_ON system("echo 1 > ")
#define FAN_PWR_OFF system("echo 0 > ")
#define MOP_UP  system("echo 1 > /sys/devices/platform/pc-switch/pc_switch/switch/do_scraper_ctl")
#define MOP_DOWN system("echo 0 > /sys/devices/platform/pc-switch/pc_switch/switch/do_scraper_ctl")

#define FAN_CTL_MAGIC 				'P'
#define FAN_CTL_SET_PERIOD			_IOW(FAN_CTL_MAGIC, 0, int)
#define FAN_CTL_GET_PERIOD			_IOR(FAN_CTL_MAGIC, 1, int)
#define FAN_CTL_SET_DUTY			_IOW(FAN_CTL_MAGIC, 2, int)
#define FAN_CTL_GET_DUTY			_IOR(FAN_CTL_MAGIC, 3, int)
#define FAN_CTL_SET_POLARITY		_IOW(FAN_CTL_MAGIC, 4, int)
#define FAN_CTL_GET_POLARITY		_IOR(FAN_CTL_MAGIC, 5, int)
#define FAN_CTL_ENABLE				_IOW(FAN_CTL_MAGIC, 6, int)
#define FAN_CTL_DISABLE				_IOR(FAN_CTL_MAGIC, 7, int)
#define FAN_CTL_GET_SPEED			_IOR(FAN_CTL_MAGIC, 8, int)
#define FAN_CTL_MAXNR					8

#define PWM_MID_MAGIC 				'P'
#define PWM_MID_SET_PERIOD			_IOW(PWM_MID_MAGIC, 0, int)
#define PWM_MID_GET_PERIOD			_IOR(PWM_MID_MAGIC, 1, int)
#define PWM_MID_SET_DUTY			_IOW(PWM_MID_MAGIC, 2, int)
#define PWM_MID_GET_DUTY			_IOR(PWM_MID_MAGIC, 3, int)
#define PWM_MID_SET_POLARITY		_IOW(PWM_MID_MAGIC, 4, int)
#define PWM_MID_GET_POLARITY		_IOR(PWM_MID_MAGIC, 5, int)
#define PWM_MID_ENABLE				_IOW(PWM_MID_MAGIC, 6, int)
#define PWM_MID_DISABLE				_IOR(PWM_MID_MAGIC, 7, int)
#define PWM_MID_MAXNR					7

#define PWM_SIDE_MAGIC 				'P'
#define PWM_SIDE_SET_PERIOD			_IOW(PWM_SIDE_MAGIC, 0, int)
#define PWM_SIDE_GET_PERIOD			_IOR(PWM_SIDE_MAGIC, 1, int)
#define PWM_SIDE_SET_DUTY			_IOW(PWM_SIDE_MAGIC, 2, int)
#define PWM_SIDE_GET_DUTY			_IOR(PWM_SIDE_MAGIC, 3, int)
#define PWM_SIDE_SET_POLARITY		_IOW(PWM_SIDE_MAGIC, 4, int)
#define PWM_SIDE_GET_POLARITY		_IOR(PWM_SIDE_MAGIC, 5, int)
#define PWM_SIDE_ENABLE				_IOW(PWM_SIDE_MAGIC, 6, int)
#define PWM_SIDE_DISABLE			_IOR(PWM_SIDE_MAGIC, 7, int)
#define PWM_SIDE_MAXNR				7

#define MOP_CTL_MAGIC 				'H'
#define MOP_CTL_SET_PERIOD			_IOW(MOP_CTL_MAGIC, 0, int)
#define MOP_CTL_SET_DUTY		    _IOW(MOP_CTL_MAGIC, 1, int)
#define MOP_CTL_SET_POLARITY		_IOW(MOP_CTL_MAGIC, 2, int)
#define MOP_CTL_ENABLE				_IOW(MOP_CTL_MAGIC, 3, int)
#define MOP_CTL_DISABLE				_IOR(MOP_CTL_MAGIC, 4, int)
#define MOP_CTL_MAXNR					4

// class MotorManager;
// typedef std::shared_ptr<MotorManager> MotorManagerPtr;

namespace peripherals{
class MotorManager : public BaseModule,
                     public chml::MessageHandler,
                     public CommParam {
public:
    explicit MotorManager(PerServer *per_server_ptr);
    virtual ~MotorManager();
    // static MotorManagerPtr &GetInstance();
    int Start();
    int Stop();
    int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
    int Set_Fan_Lev(Json::Value &jreq, Json::Value &jret);
    int Set_Mid_Lev(Json::Value &jreq, Json::Value &jret);
    int Set_Side_Lev(Json::Value &jreq, Json::Value &jret);
    int Set_Mop_Lev(Json::Value &jreq, Json::Value &jret);//拖布电机控制，PWM控制无刷电机调速或者GPIO控制刮条电机

    void Fan_Motor_Run();
    void Mid_Motor_Run();
    void Side_Motor_Run();
    void Mop_Motor_Run();
    void Ultra_Sample();
    void Timer50ms();
private:
    // static MotorManagerPtr instance_;
    int fan_fd;//风机的设备操做符
    int mid_fd;//中扫的设备操作符
    int side_fd;//边扫的设备操作符
    int mop_fd; // 拖布电机的设备操作符
    int ultra_fd; //超声波设备操做符
    int mop_cnt;
private:
    void OnMessage(chml::Message* msg);
    chml::EventService::Ptr Event_Service_;
    uint8_t ultra_buf[13];
};

}

#endif

