#include "control_interface.hpp"
#include "eventservice/dp/dpclient.h"
#include "robot_schedule/schedule/SensorData.h"
#include "robot_schedule/base/common.h"
//#include "Peripheral/device_manager.h"
//DeviceManagerPtr dev_manager;
// = DeviceManager::GetInstance();
//工作中断变量
//bool interrupt_ = false;
//bool work_pause_ = false;
//停止工作接口
void  interrupt_work()
{
    //interrupt_ = true;
    set_ML_pause(2);
    printf("1112222333333\n");
}

//暂停工作
void pause_work()
{
    
    set_ML_pause(1);
}
//恢复工作接口
void resume_work()
{
    set_ML_pause(0);
}

//全屋清洁接口
//清洗拖布、出仓由外部完成
//void house_clean()
//{
//    set_ML_clean(1);
//    control_thread();
//}
uint8_t house_clean(char from_base,char map_id,char cotinue_clean)
{
    set_ML_clean(1);
    set_ML_init_state(from_base);
    set_ML_load_map_id(map_id);
    set_ML_continue_clean(cotinue_clean);
    control_thread();
}
uint8_t room_clean(ROOM_NAME room,char from_base)
{
   set_ML_clean(2);
   set_ML_room(room);
   set_ML_init_state(from_base);
}

uint8_t area_clean(Point bottomLeftCorner ,Point topRightCorner)
{

}


void setVWtoRobot(int16_t v,int16_t w)
{
    chml::DpClient::Ptr sensor_client_ = SensorData::getsensorData()->GetDpsensorClientPtr();
    DLOG_INFO(MOD_BUS, "CmdGoForward");
    std::string str_req, str_res;
    Json::Value json_req;
    json_req[JKEY_TYPE] = "set_v_w";
    json_req[JKEY_BODY][JKEY_BRAKE] = 0;
    json_req[JKEY_BODY][JKEY_V] = v;
    json_req[JKEY_BODY][JKEY_W] = w;
    Json2String(json_req, str_req);
    sensor_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
}
void set_forbidden_zone(std::vector<Room> &forbidden_info)
{
    for(int i = 0;i < forbidden_info.size();i++)
    {
        Rectangle rec;
        rec.lb.x = forbidden_info[i].vertexs_value[0][0];
        rec.lb.y = forbidden_info[i].vertexs_value[0][1];
        rec.lt.x = forbidden_info[i].vertexs_value[1][0];
        rec.lt.y = forbidden_info[i].vertexs_value[1][1];
        rec.rb.x = forbidden_info[i].vertexs_value[2][0];
        rec.rb.y = forbidden_info[i].vertexs_value[2][1];
        rec.rt.x = forbidden_info[i].vertexs_value[3][0];
        rec.rt.y = forbidden_info[i].vertexs_value[3][1];
        generate_forrbiden_map(rec);  
    }

}