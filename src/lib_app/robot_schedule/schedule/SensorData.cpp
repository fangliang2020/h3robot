//#include "Peripheral/device_manager.h"
#include "SensorData.h"
#include "robot_schedule/clean/config.hpp"
#include "robot_schedule/socket/navigate.h"
#include <map>
#include <iostream>
std::mutex SensorData::m_sensorMutex;
SensorData *SensorData::m_sensordata = nullptr;

SensorData::SensorData()
{

    STM_KEY_VALUE = -1;
    fd = -2;
    ret = 0;
    key_data[16] = {0};
    m_sensor = true;
    network_value = -2;
    dp_sensor_client_ = nullptr;
    dp_key_client_ = nullptr;
    dp_fault_client_ = nullptr;
    Mysensor_Service_ = nullptr;

   
}

SensorData::~SensorData()
{
    if (m_sensordata)
    {
        delete(m_sensordata);
    }
    m_sensordata = nullptr;
    printf("deleat sensorth\n");
    
    
}

void SensorData::set_sensor_data(bool sensor_value)
{
    m_sensor = sensor_value;

}
bool SensorData::get_sensor_data()
{
    return  m_sensor;
}

SensorData *SensorData::getsensorData()
{
    if (m_sensordata == nullptr)
    {
        m_sensorMutex.lock();
        if (m_sensordata == nullptr)
        {
            m_sensordata = new SensorData();
        }
        m_sensorMutex.unlock();    
        
    }

    return m_sensordata;
}
SensorData *SensorData::delsensorData()
{
    if (m_sensordata)
    {
       
        delete(m_sensordata);
        m_sensordata = nullptr;
    }
    
}
int SensorData::getnetWorkValue()
{
    return network_value;
}
void SensorData::setnetWorkValue(int &net_value)
{
    network_value = net_value;
}
void SensorData::set_fan_level(int &fan_level)
{
     
    DLOG_INFO(MOD_BUS, "set_fan_level");
    std::string str_req, str_res;
    Json::Value json_req;
    json_req[JKEY_TYPE] = "set_fan_level";
    json_req[JKEY_BODY]["level"] = fan_level;
    Json2String(json_req, str_req);
    dp_sensor_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
}
void SensorData::set_mid_level(int &mid_level)
{
    DLOG_INFO(MOD_BUS, "set_mid_level");
    std::string str_req, str_res;
    Json::Value json_req;
    json_req[JKEY_TYPE] = "set_mid_level";
    json_req[JKEY_BODY]["level"] = mid_level;
    Json2String(json_req, str_req);
    dp_sensor_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
}
void SensorData::set_side_level(int &side_level)
{
    DLOG_INFO(MOD_BUS, "set_side_level");
    std::string str_req, str_res;
    Json::Value json_req;
    json_req[JKEY_TYPE] = "set_side_level";
    json_req[JKEY_BODY]["level"] = side_level;
    Json2String(json_req, str_req);
    dp_sensor_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
}
void SensorData::set_mop_ctrl(int &mop_ctrl)
{
    DLOG_INFO(MOD_BUS, "set_mop_ctrl");
    std::string str_req, str_res;
    Json::Value json_req;
    json_req[JKEY_TYPE] = "set_mop_ctrl";
    json_req[JKEY_BODY]["ctrl"] = mop_ctrl;
    Json2String(json_req, str_req);
    dp_sensor_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
}
void SensorData::set_voice_play(std::string play_voice)
{
    DLOG_INFO(MOD_BUS, "set_voice");
    std::string str_req, str_res;
    Json::Value json_req;
    json_req[JKEY_TYPE] = "set_voice";
    json_req[JKEY_BODY]["voice_tts"] = play_voice;
    Json2String(json_req, str_req);
    dp_sensor_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
}
    

chml::DpClient::Ptr SensorData::GetDpsensorClientPtr(){
  return dp_sensor_client_;
}
void SensorData::res_periphe_sensor(std::string &sentype,Json::Value& sjreq)
{
    if (sentype == "sensor_data")
    {
        //std::cout << "sensor_data is get" << std::endl;
        auto JV_TO_INT(sjreq[JKEY_BODY]["vl6180"], JKEY_DIS, dis, 0);
        set_followwall_ir_data(dis);//沿墙
        //std::cout << "the ir dis is" << dis <<std::endl;
        auto JV_TO_DOUBLE(sjreq[JKEY_BODY]["imu"], JKEY_X_ACCEL, x_accel, 0);
        auto JV_TO_DOUBLE(sjreq[JKEY_BODY]["imu"], JKEY_Y_ACCEL, y_accel, 0);
        auto JV_TO_DOUBLE(sjreq[JKEY_BODY]["imu"], JKEY_Z_ACCEL, z_accel, 0);
        auto JV_TO_DOUBLE(sjreq[JKEY_BODY]["imu"], JKEY_X_GYRO, x_gyro, 0);
        auto JV_TO_DOUBLE(sjreq[JKEY_BODY]["imu"], JKEY_Y_GYRO, y_gyro, 0);
        auto JV_TO_DOUBLE(sjreq[JKEY_BODY]["imu"], JKEY_Z_GYRO, z_gyro, 0);
        auto JV_TO_DOUBLE(sjreq[JKEY_BODY]["imu"], JKEY_COURSE_ANGLE, course_angle, 0);
        imu_callback(course_angle,0,0,x_accel,y_accel,z_accel);//imu
        //std::cout << "the imu is" << course_angle <<std::endl;
        auto JV_TO_INT(sjreq[JKEY_BODY]["odm"], JKEY_ODM_LEFT, odm_left, 0);
        auto JV_TO_INT(sjreq[JKEY_BODY]["odm"], JKEY_ODM_RIGHT, odm_right, 0);
        // std::cout << "the odm left is" << odm_left <<std::endl;
        // std::cout << "the odm right is" << odm_right <<std::endl;
        odom_callback(odm_left,odm_right);//里程计
        auto JV_TO_INT(sjreq[JKEY_BODY][JKEY_COLLISION], JKEY_COLLISION_LF, collision_lf, 0);
        auto JV_TO_INT(sjreq[JKEY_BODY][JKEY_COLLISION], JKEY_COLLISION_LB, collision_lb, 0);
        auto JV_TO_INT(sjreq[JKEY_BODY][JKEY_COLLISION], JKEY_COLLISION_RF, collision_rf, 0);
        auto JV_TO_INT(sjreq[JKEY_BODY][JKEY_COLLISION], JKEY_COLLISION_RB, collision_rb, 0);
        set_impact_ir_data(collision_lf,collision_lb,collision_rf,collision_rb);//碰撞
        JV_TO_UINT(sjreq[JKEY_COLLISION]["collision"], JKEY_COLLISION_LF, collision_lf, -1);
        // if (sjreq[JKEY_BODY]["vl53l5cx"]["data"].isArray())
        // {
        //     int vl53l5cx_size = sjreq[JKEY_BODY]["vl53l5cx"]["data"].size();
        //     int vl53l5cx_x=13,vl53l5cx_y=0;
        //     int j = 0;
        //     for (int i = 0; i < vl53l5cx_size; i++)
        //     {
        //         auto vl53l5cx_dis = sjreq[JKEY_BODY]["vl53l5cx"]["data"][i].asUInt();
        //         if (i % 8 == 0)
        //         {
        //             j++;
        //             vl53l5cx_x--;
        //         }
        //         if (j % 2 != 0)//奇数列
        //         {
        //             set_tof_data(vl53l5cx_x,vl53l5cx_y,vl53l5cx_dis);
        //             //std::cout <<"X is "<< vl53l5cx_x <<" Y is "<<vl53l5cx_y<<" dis is "<<vl53l5cx_dis<< std::endl;
        //             vl53l5cx_y++;
        //         }
                
        //         if (j % 2 == 0)
        //         {
        //             set_tof_data(vl53l5cx_x,vl53l5cx_y,vl53l5cx_dis);
        //             //std::cout <<"X is "<< vl53l5cx_x <<" Y is "<<vl53l5cx_y<<" dis is "<<vl53l5cx_dis<< std::endl;
        //             vl53l5cx_y--;
        //         }
                
                
        //     }
            
        // }
        
        //set_tof_data(vl53l5cx_tofdata->tof_vector[i].x,vl53l5cx_tofdata->tof_vector[i].y,vl53l5cx_tofdata->tof_vector[i].dis);

    }
 
}
void SensorData::res_periphe_key(std::string &ksentype,Json::Value& ksjreq)
{
    
    if (ksentype == "trigger_data")
    {
        //std::cout << "key_data is get" << std::endl;
        uint8_t JV_TO_UINT(ksjreq[JKEY_BODY], "buttons", buttons_value, 0);
        key_value = buttons_value;
        //std::cout << "key_value is" << key_value << std::endl;
    }
    
    auto mainState = CDataModel::getDataModel()->getMState();
    
    if (network_value != -1) //配网中按键无法触发工作
    {
        if (key_value == 1 && mainState == ESTATE_IDLE)
        {
            CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_GLOBAL_CLEAN);
            
            key_value = 0;
            std::cout << "key global clean" << std::endl;
        }
        if (key_value == 1 && mainState == ESTATE_CLEAN)
        {
            CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_PAUSE_CLEAN);
            CDataModel::getDataModel()->setMState(ESTATE_PAUSE_CLEAN);
            set_voice_play(PLAY_Q007);
            pause_work();
            key_value = 0;
            
            CRobotSchedule::getRobotSchedule()->sendState();
            std::cout << "key pause clean" << std::endl;
        }
        if (key_value == 1 && mainState == ESTATE_PAUSE_CLEAN)
        {
            CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_RESUME_CLEAN);
            CDataModel::getDataModel()->setMState(ESTATE_CLEAN);
            set_voice_play(PLAY_Q008);
            resume_work();
            key_value = 0;
            CRobotSchedule::getRobotSchedule()->sendState();
            std::cout << "key resume clean" << std::endl;
        }
    }
    if (key_value == 2)//关机
    {
        //广播状态
        std::cout << "the show_dow_value is" << key_value << std::endl;
        int close = 0;
        set_mop_ctrl(close);
        CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_SHUTDOWM);
        CDataModel::getDataModel()->setMState(ESTATE_SHUTDOWM);
        set_voice_play(PLAY_Q022);
        key_value = 0;
        CRobotSchedule::getRobotSchedule()->sendState();
    }
    if (key_value == 4)
    {
        pause_work();
        key_value = 0;
        network_value = -1;
    }
    
    
}
void SensorData::res_periphe_fault(std::string &faulttype,Json::Value& fsjreq)
{
    if (faulttype == "fault_state")
    {
        
        if (CDataModel::getDataModel()->getMState() != ESTATE_ERROR)
        {
            int JV_TO_UINT(fsjreq[JKEY_BODY], "fault_state", fault_state, 0);
            // CDataModel::getDataModel()->setMState(ESTATE_ERROR);
            // pause_work();
            // for (int i = 0; i < 25; i++)
            // {
            //     auto fault_value = fault_state&(1<<i);
            //     if (fault_value == 1)
            //     {
            //         //CDataModel::getDataModel()->setFState(i+1);
                    
            //     }
                    
            // }
            
            if (fault_state)
            {
                CDataModel::getDataModel()->setMState(ESTATE_ERROR);
                pause_work();
                CRobotSchedule::getRobotSchedule()->sendState();

            }
            
            
        }
            /*********1,新的异常触发 2,解除异常*********/
        if (CDataModel::getDataModel()->getMState() == ESTATE_ERROR)
        {
            auto first_state = CDataModel::getDataModel()->getMQueueFort();
            int JV_TO_UINT(fsjreq[JKEY_BODY], "fault_state", fault_state, 0);
            if (fault_state == 0)
            {
                switch (first_state)
                {
                case ESTATE_CLEAN:
                    CDataModel::getDataModel()->setMState(ESTATE_PAUSE_CLEAN);
                    CRobotSchedule::getRobotSchedule()->sendState();
                    break;
                case ESTATE_FAST_BUILD_MAP:
                    CDataModel::getDataModel()->setMState(ESTATE_PAUSE_FAST_BUILD_MAP);
                    CRobotSchedule::getRobotSchedule()->sendState();
                    break;
                case ESTATE_BACK_CHARGE:
                    CDataModel::getDataModel()->setMState(ESTATE_CHAGRE_PAUSE);
                    CRobotSchedule::getRobotSchedule()->sendState();
                    break;
                
                default:
                    break;
                }
                
            }
            
           
                 
                    
        }

    }
}
    
    
    
   


void SensorData::OnDpSensor_Message(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg) {
  //int ret=-1;
  std::string stype;
  Json::Value jreq;//jret; //广播消息无需回复，去掉jret
  DLOG_DEBUG(MOD_EB,"=== dp %s",dp_msg->req_buffer.c_str());
  if(dp_msg->method=="DP_PERIPHERALS_BROADCAST") 
  {
    if (!String2Json(dp_msg->req_buffer, jreq)) {
      DLOG_ERROR(MOD_EB, "json parse failed.");
      return ;
    }
    stype = jreq[JKEY_TYPE].asString();
    SensorData::res_periphe_sensor(stype,jreq);

  } 


}
void SensorData::OnDpKey_Message(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg)
{
  //int ret_key=-1;
  std::string kstype;
  Json::Value kjreq;//kjret; //广播消息无需回复，去掉kjret
  DLOG_DEBUG(MOD_EB,"=== dp %s",dp_msg->req_buffer.c_str());
  if(dp_msg->method=="DP_PERIPHERALS_BROADCAST") 
  {
    if (!String2Json(dp_msg->req_buffer, kjreq)) {
      DLOG_ERROR(MOD_EB, "json parse failed.");
      return ;
    }
    kstype = kjreq[JKEY_TYPE].asString();
    SensorData::res_periphe_key(kstype,kjreq);

  } 
}
void SensorData::OnDpFault_Message(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg)
{
    std::string fstype;
    Json::Value fjreq;
    DLOG_DEBUG(MOD_EB,"=== dp %s",dp_msg->req_buffer.c_str());
    if(dp_msg->method=="DP_PERIPHERALS_BROADCAST") 
    {
        if (!String2Json(dp_msg->req_buffer, fjreq)) {
        DLOG_ERROR(MOD_EB, "json parse failed.");
        return ;
    }
    fstype = fjreq[JKEY_TYPE].asString();
    SensorData::res_periphe_fault(fstype,fjreq);
    } 
}
bool SensorData::init()
{
    std::cout << "SensorData init start" <<std::endl;
    Mysensor_Service_=chml::EventService::CreateEventService(NULL, "sensor_service");
    chml::SocketAddress dp_per_addr(ML_DP_SERVER_HOST, ML_DP_SERVER_PORT);
    dp_sensor_client_=chml::DpClient::CreateDpClient(Mysensor_Service_,dp_per_addr,"dp_sensor_service");
    dp_key_client_=chml::DpClient::CreateDpClient(Mysensor_Service_,dp_per_addr,"dp_key_service");
    dp_fault_client_=chml::DpClient::CreateDpClient(Mysensor_Service_,dp_per_addr,"dp_fault_service");
    ASSERT_RETURN_FAILURE(!dp_sensor_client_, false);
    ASSERT_RETURN_FAILURE(!dp_key_client_, false);
    ASSERT_RETURN_FAILURE(!dp_fault_client_, false);
    dp_sensor_client_->SignalDpMessage.connect(this,&SensorData::OnDpSensor_Message);
    dp_key_client_->SignalDpMessage.connect(this,&SensorData::OnDpKey_Message);
    dp_fault_client_->SignalDpMessage.connect(this,&SensorData::OnDpFault_Message);
    if (dp_sensor_client_->ListenMessage("DP_PERIPHERALS_BROADCAST", chml::TYPE_MESSAGE)) {
    DLOG_ERROR(MOD_EB, "dp client listen sensor_client message failed.");
     }
    if (dp_key_client_->ListenMessage("DP_PERIPHERALS_BROADCAST", chml::TYPE_MESSAGE)) {
    DLOG_ERROR(MOD_EB, "dp client listen key_client message failed.");
     }
    // if (dp_fault_client_->ListenMessage("DP_PERIPHERALS_BROADCAST", chml::TYPE_MESSAGE)) {
    // DLOG_ERROR(MOD_EB, "dp client listen fault_client message failed.");
    //  }
    sleep(1);
    std::cout << "SensorData init success" <<std::endl;
    return true;

}