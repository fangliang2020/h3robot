#ifndef SENSOR_DATA
#define SENSOR_DATA

#include "DateModel.h"
#include "RobotSchedule.h"
#include "robot_schedule/clean/control_interface.hpp"
#include <iostream>
#include <string>
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"
#include "sys/select.h"
#include "sys/time.h"
#include "linux/ioctl.h"
#include "../base/base_module.h"
#include "json/json.h"
#include "net_server/base/jkey_common.h"
//#include "PlayVoice.h"
#include "peripherals_server/audio/voice_play_config.h"
class SensorData : public sigslot::has_slots<>
//: public app::AppInterface,
                  //public boost::enable_shared_from_this<SensorData>,
                 // public sigslot::has_slots<>
{
private:
    
    // std::thread *m_sensoralTh;
    // std::thread *m_sensorcoTh;
    // std::thread *m_sensorwhellTh;
    // std::thread *m_sensorkeyTh;
    // std::thread *m_sensorimuTh;
    // std::thread *m_sensortofTh;
    static SensorData *m_sensordata;
    int STM_KEY_VALUE;
    int fd;
    int ret;
    uint8_t key_value;
    //uint8_t fault_value;
    unsigned char key_data[16];
    bool m_sensor;
    int network_value;
    chml::DpClient::Ptr dp_sensor_client_;
    chml::DpClient::Ptr dp_key_client_;
    chml::DpClient::Ptr dp_fault_client_;
    chml::EventService::Ptr Mysensor_Service_;
    SensorData();
    

public:
    //typedef boost::shared_ptr<SensorData> Ptr;
    //typedef std::vector<BaseModule*> VecBaseMdule;
    ~SensorData();
    static std::mutex m_sensorMutex;
    static SensorData *getsensorData();
    static SensorData *delsensorData();
    bool init();
    // void Sensor_aldata();
    // int Sensor_codata();
    // void Sensor_wheeldata();
    // void Sensor_keydata();
    // void Sensor_imudata();
    // void Sensor_tofdata();
    int getnetWorkValue();
    void setnetWorkValue(int &net_value);
    bool get_sensor_data();
    void set_sensor_data(bool sensor_value);
    void OnDpSensor_Message(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg);
    void OnDpKey_Message(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg);
    void OnDpFault_Message(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg);
    void res_periphe_sensor(std::string &sentype,Json::Value& sjreq);
    void res_periphe_key(std::string &ksentype,Json::Value& ksjreq);
    void res_periphe_fault(std::string &faulttype,Json::Value& fsjreq);
    void set_fan_level(int &fan_level);//风机控制
    void set_mid_level(int &mid_level);//中扫控制
    void set_side_level(int &side_level);//边扫控制
    void set_mop_ctrl(int &mop_ctrl);//拖布升降控制
    void set_voice_play(std::string play_voice);
    chml::DpClient::Ptr GetDpsensorClientPtr();
   
    
};

#endif