#include "NetserverInstruct.h"
#include <iostream>
std::mutex NetserverInstruct::m_netMutex;
NetserverInstruct *NetserverInstruct::m_netinstruct = nullptr;
NetserverInstruct::~NetserverInstruct()
{
    if (m_netinstruct)
    {
        delete m_netinstruct;
        m_netinstruct = nullptr;
    }
    if (net_timerTh)
    {
        net_timerTh->join();
        delete net_timerTh;
    }
    
    
}
NetserverInstruct::NetserverInstruct()
{
    dp_netInstruct_client_ = nullptr;
    MynetInstruct_Service_ = nullptr;
    net_timerTh = nullptr;
    work_time = -1;
}
NetserverInstruct *NetserverInstruct::getNetInstruct()
{

    if (m_netinstruct == nullptr)
    {
        m_netMutex.lock();
        if (m_netinstruct == nullptr)
        {
            m_netinstruct = new NetserverInstruct();
        }
        m_netMutex.unlock();
        
    }
    
    return m_netinstruct;
}
chml::DpClient::Ptr NetserverInstruct::GetDpnetserverClientPtr()
{
    return dp_netInstruct_client_;
}
void NetserverInstruct::time_reckon()
{
    while(true)
	{
		net_timer.DetectTimers();
		usleep(30*1000);
        //std::cout << "------------------------"<< std::endl;
         if (CRobotSchedule::getRobotSchedule()->getCRobotdata() == false)
         {
             break;
         }
        
	}
}
void NetserverInstruct::send_clean_log()
{
	DLOG_INFO(MOD_BUS, "send_clean_log");
	std::string str_req, str_res;
	Json::Value json_req;
    DayTime log_time;
    int i = 0;
    char start_time[24] = {0};
    snprintf(start_time,24,"%d-%d-%d %d:%d",log_time.time_year,log_time.time_mon,log_time.time_wday,log_time.time_hour,log_time.time_min);
    auto clean_area = get_clean_area();
    int clean_time = get_clean_time();
    int m_clean_type = CRobotSchedule::getRobotSchedule()->getcleantype();
    int m_clean_result = CRobotSchedule::getRobotSchedule()->getcleanresult();
    if (!pose_vector.empty())
    {
        for (auto& pose:pose_vector)
        {
            json_req[JKEY_BODY]["log"]["clean_trail"][i][0] = pose.pose_x;
            json_req[JKEY_BODY]["log"]["clean_trail"][i][1] = pose.pose_y;
            i++;
        }
    }
	json_req[JKEY_TYPE] = "clean_log";
	json_req[JKEY_BODY]["log"]["map_id"] = 1;
	json_req[JKEY_BODY]["log"]["clean_start_time"] = start_time;
    json_req[JKEY_BODY]["log"]["map_info_path"] = "/userdata/map/housemap_info.txt";
    json_req[JKEY_BODY]["log"]["map_data_path"] = "/userdata/map/housemap.txt";
    json_req[JKEY_BODY]["log"]["clean_area"] = clean_area;
    json_req[JKEY_BODY]["log"]["clean_time"] = clean_time;
    json_req[JKEY_BODY]["log"]["clean_type"] = m_clean_type;
    json_req[JKEY_BODY]["log"]["clean_result"] = m_clean_result;
	Json2String(json_req, str_req);
	dp_netInstruct_client_->SendDpMessage(DP_SCHEDULE_BROADCAST, 0, str_req.c_str(), str_req.size(), &str_res);
}
void NetserverInstruct::pose_down_net(int &pose_x,int &pose_y,int &pose_yam)
{
    //time_t now_time = time(NULL);
    // if (work_time < 0)
    // {
        //work_time = now_time;
        if (pose_vector.size()>10000)
        {
        
            pose_vector.erase(pose_vector.begin());
        }
        struct robot_pose pose;
        pose.pose_x = double(pose_y)/50;
        pose.pose_y = double(-pose_x)/50;
        pose.pose_yam = pose_yam/1000;
        std::cout<<"the robot pose x" << pose.pose_x << std::endl;
        std::cout<<"the robot pose y" << pose.pose_y<< std::endl;
        std::cout<<"the robot pose angle"<< pose_yam<< std::endl;
        pose_vector.emplace_back(pose);
    //}
    // else if (now_time - work_time >1)
    // {
    //     work_time = now_time;
    //     if (pose_list.size()>10000)
    //     {
            
    //         pose_list.pop_front();
    //     }
    //     struct robot_pose pose;
    //     pose.pose_x = pose_x;
    //     pose.pose_y = pose_y;
    //     pose.pose_yam = pose_yam/1000;
    //     std::cout<<"the robot pose x" << pose_x << std::endl;
    //     std::cout<<"the robot pose y" << pose_y<< std::endl;
    //     std::cout<<"the robot pose angle"<< pose_yam<< std::endl;
    //     pose_list.push_back(pose);
    // }
    // else
    return;
    
}
// void NetserverInstruct::send_clean_trail()
// {
//     int i = 0;
//     std::string str_req, str_res;
// 	Json::Value json_req;
//     if (pose_vector.empty())
//     {
//         return;
//     }
//     DLOG_INFO(MOD_BUS, "send_clean_trail");
//     json_req[JKEY_TYPE] = "update_clean_trail";
//     for (auto& pose:pose_list)
//     {
//         json_req[JKEY_BODY]["clean_trail"][i][0] = pose.pose_x;
//         json_req[JKEY_BODY]["clean_trail"][i][1] = pose.pose_y;
//         //json_req["robot_traj"][i][2] = pose.pose_yam;
//         i++;
//     }
//     Json2String(json_req, str_req);
//     dp_netInstruct_client_->SendDpMessage(DP_SCHEDULE_BROADCAST, 0, str_req.c_str(), str_req.size(), &str_res);
// }
void NetserverInstruct::send_robot_pose()
{
 
    if (pose_vector.empty())
    {
        return;
    }
    
    struct robot_pose& pose = pose_vector.back();
	DLOG_INFO(MOD_BUS, "send_robot_pose");
	std::string str_req, str_res;
	Json::Value json_req;
	json_req[JKEY_TYPE] = "robot_device_pose";
	json_req[JKEY_BODY]["clean_robot_pos"][0] = pose.pose_x;
	json_req[JKEY_BODY]["clean_robot_pos"][1] = pose.pose_y;
	json_req[JKEY_BODY]["clean_robot_pos"][2] = pose.pose_yam;
    json_req[JKEY_BODY]["charge_base_pos"][0] = 0;
    json_req[JKEY_BODY]["charge_base_pos"][1] = 0;
	Json2String(json_req, str_req);
	dp_netInstruct_client_->SendDpMessage(DP_SCHEDULE_BROADCAST, 0, str_req.c_str(), str_req.size(), &str_res);
}
void NetserverInstruct::send_clean_area()
{
    if (CDataModel::getDataModel()->getMState() == ESTATE_CLEAN)
    {
        DLOG_INFO(MOD_BUS, "send_clean_area_cfg");
        auto clean_area = get_clean_area();
        int clean_time = get_clean_time();
        std::cout << "the clean area is" << clean_area << std::endl;
        std::string str_req, str_res;
        Json::Value json_req;
        json_req[JKEY_TYPE] = "update_clean_area_cfg";
        json_req[JKEY_BODY]["area"] = clean_area;
        json_req[JKEY_BODY]["time"] = clean_time;
        
        Json2String(json_req, str_req);
        dp_netInstruct_client_->SendDpMessage(DP_SCHEDULE_BROADCAST, 0, str_req.c_str(), str_req.size(), &str_res);
    }
    
   
}
void NetserverInstruct::netserverIntergrationHandle(EJobCommand job_cmd,E_MASTER_STATE m_state,std::string play_voice)
{
    CRobotjobEvent::getjobEvent()->setJobCommand(job_cmd);
    CDataModel::getDataModel()->setMState(m_state);
    SensorData::getsensorData()->set_voice_play(play_voice);
    CRobotSchedule::getRobotSchedule()->sendState();
}
bool NetserverInstruct::init()
{
    std::cout << "NetserverInstruct init start" <<std::endl;
    MynetInstruct_Service_=chml::EventService::CreateEventService(NULL, "netinstruct_service");
    chml::SocketAddress dp_per_addr(ML_DP_SERVER_HOST, ML_DP_SERVER_PORT);
    dp_netInstruct_client_=chml::DpClient::CreateDpClient(MynetInstruct_Service_,dp_per_addr,"dp_netInstruct_service");
    ASSERT_RETURN_FAILURE(!dp_netInstruct_client_, false);
    dp_netInstruct_client_->SignalDpMessage.connect(this,&NetserverInstruct::OnDpNet_Message);
    //接收到消息后需要回复request,相当于注册一个 "DP_PERIPHERALS_REQUEST"服务，都可以被其他进程调用
    if (dp_netInstruct_client_->ListenMessage("DP_SCHEDULE_REQUEST", chml::TYPE_REQUEST)) {
    DLOG_ERROR(MOD_EB, "dp client listen message failed.");
     }
    net_timer.register_timer([](uint64_t){NetserverInstruct::getNetInstruct()->send_robot_pose();}, 3000, 1000, REPEAT_FOREVER);
    net_timer.register_timer([](uint64_t){NetserverInstruct::getNetInstruct()->send_clean_area();}, 4000, 1000, REPEAT_FOREVER);
    net_timerTh = new std::thread(std::bind(&NetserverInstruct::time_reckon,this));
    sleep(1);
    std::cout << "NetserverInstruct init success" <<std::endl;
    return true;

}
void NetserverInstruct::OnDpNet_Message(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg)
{
    int ret=-1;
  std::string stype;
  Json::Value jreq,jret;
  DLOG_DEBUG(MOD_EB,"=== dp %s",dp_msg->req_buffer.c_str());
  if(dp_msg->method=="DP_SCHEDULE_REQUEST") 
  {
    if (!String2Json(dp_msg->req_buffer, jreq)) {
      DLOG_ERROR(MOD_EB, "json parse failed.");
      return;
    }
    stype = jreq[JKEY_TYPE].asString();
    NetserverInstruct::work_handle(stype,jreq);
    NetserverInstruct::workstate_handle(stype, jret[JKEY_BODY], jreq);

  } 
  if(dp_msg->type == chml::TYPE_REQUEST) {
    jret[JKEY_TYPE]=stype;
    if(ret==0) {
      jret[JKEY_STATUS]=200;
      jret["err_msg"]="success";
    }
    else {
      // jret[JKEY_STATUS]=400;
      // jret[JKEY_ERR]="error";
    }
    Json2String(jret, *dp_msg->res_buffer);
    DLOG_INFO(MOD_EB, "ret %s", (*dp_msg->res_buffer).c_str());
    if(dp_msg->type == chml::TYPE_REQUEST)
    {
        //printf("the JKEY_TYPE is %s,the msg is %d\n",stype.c_str(),(*dp_msg->res_buffer).size());
    }
    
    dp_cli->SendDpReply(dp_msg);
  }
}
bool NetserverInstruct::work_handle(std::string &netype,Json::Value& jreq)
{
    
    if (netype == "fast_build_map")
    {
        CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_FAST_BUILD_MAP);
    }

    // if (netype == "edit_map_name") 修改地图名字
    // {
    //     /* code */
    // }
    if (netype == "recharge")
    {
        CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_BACK_CHARGE);
    }
    
    if (netype == "start_clean")
    {
        CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_GLOBAL_CLEAN);
    }
    if (netype == "zoning_clean")
    {
        int JV_TO_INT(jreq[JKEY_BODY]["extra_areas"], "mode", mode, 0);
        switch (mode)
        {
        case 1: //划区清扫
            CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_LOCAL_CLEAN);
            break;
        case 2: //定点清扫（房间）
            CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_ZONE_CLEAN);
            break;
        case 3: //指哪儿扫哪儿
            CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_POINT_CLEAN);
            break;
        default:
            break;
        }
        
    }
    if (netype == "pause_clean")
    {
        
        auto now_state = CDataModel::getDataModel()->getMState();
        switch (now_state)
        {
        case ESTATE_CLEAN:
            pause_work();
            netserverIntergrationHandle(EJOB_PAUSE_CLEAN,ESTATE_PAUSE_CLEAN,PLAY_Q007);
            break;
        case ESTATE_FAST_BUILD_MAP:
            pause_work();
            netserverIntergrationHandle(EJOB_PAUSE_CLEAN,ESTATE_PAUSE_FAST_BUILD_MAP,PLAY_Q007);
            break;
        case ESTATE_BACK_CHARGE:
            pause_work();
            netserverIntergrationHandle(EJOB_PAUSE_CLEAN,ESTATE_CHAGRE_PAUSE,PLAY_Q007);
            break;
        case ESTATE_BACK_WASH:
            pause_work();
            netserverIntergrationHandle(EJOB_PAUSE_CLEAN,ESTATE_WASH_PAUSE,PLAY_Q007);
            break;
        default:
            break;
        }
        
    }
    if (netype == "continue_clean")
    {
        auto work_state = CDataModel::getDataModel()->getMState();
        switch (work_state)
        {
        case ESTATE_PAUSE_CLEAN:
            resume_work();
            netserverIntergrationHandle(EJOB_RESUME_CLEAN,ESTATE_CLEAN,PLAY_Q008);
            break;
        case ESTATE_PAUSE_FAST_BUILD_MAP:
            resume_work();
            netserverIntergrationHandle(EJOB_RESUME_CLEAN,ESTATE_FAST_BUILD_MAP,PLAY_Q008);
            break;
        case ESTATE_CHAGRE_PAUSE:
            resume_work();
            netserverIntergrationHandle(EJOB_RESUME_CLEAN,ESTATE_BACK_CHARGE,PLAY_Q008);
            break;
        case ESTATE_WASH_PAUSE:
            resume_work();
            netserverIntergrationHandle(EJOB_RESUME_CLEAN,ESTATE_BACK_WASH,PLAY_Q008);
            break;
        
        default:
            break;
        }
        

    }
    if (netype == "stop_clean")
    {
        interrupt_work();
        CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_STOP_CLEAN);
        CDataModel::getDataModel()->setMState(ESTATE_IDLE);
        CRobotSchedule::getRobotSchedule()->sendState();
    }
    if (netype == "select_map") //选择地图
    {
        int JV_TO_INT(jreq[JKEY_BODY], "map_id", map_id, 0);
        switch (map_id)
        {
        case 1:
        {
            struct map_update map_data;
            map_data.map_id = 1;
            map_data.map_info_path = "/userdata/map/housemap_info.txt";
            map_data.map_data_path = "/userdata/map/housemap.txt";
            break;
        }
        case 2:
        {
            struct map_update map_data;
            map_data.map_id = 2;
            map_data.map_info_path = "/userdata/map2/housemap_info2.txt";
            map_data.map_data_path = "/userdata/map2/housemap2.txt";
            break;
        }
        case 3:
        {
            struct map_update map_data;
            map_data.map_id = 3;
            map_data.map_info_path = "/userdata/map3/housemap_info3.txt";
            map_data.map_data_path = "/userdata/map3/housemap3.txt";
            break;
        }
        default:
            break;
        }
    }
    if(netype == "clear_robot_traj")
    {
        if (pose_vector.empty())
        {
            return false;
        }
       
        {
        m_netMutex.lock();
        vector<robot_pose>().swap(pose_vector);
        m_netMutex.unlock();
        }

        
        
    }
    if (netype == "set_wifi_state")
    {
        int JV_TO_INT(jreq[JKEY_BODY], "wifi_state", wifi_state, 0);
        if (wifi_state == 1 || wifi_state == 0)
        {
            SensorData::getsensorData()->setnetWorkValue(wifi_state); //配网成功或失败都可以按键触发清扫
        }
        
    }
    
    return true;
}
void NetserverInstruct::workstate_handle(std::string &state_type, Json::Value& jres, Json::Value& jreq)
{
    if (state_type == "read_current_map")
    {
        jres["map_id"] = 1;
        jres["map_info_path"] = "/userdata/map/housemap_info.txt";
        jres["map_data_path"] = "/userdata/map/housemap.txt";
    }
    
    if (state_type == "get_all_state")
    {
        jres["current_map_id"] = 1;
        jres["device_state"] = CDataModel::getDataModel()->getMState();
        jres["clean_state"] = CDataModel::getDataModel()->getCleanState();
        jres["clean_area"] = 122; // TODO 
        jres["clean_time"] = 21;
        jres["self_clean_progress"] = 0;
        
        if (pose_vector.empty())
        {
            jres["clean_robot_pos"][0] = 1;
            jres["clean_robot_pos"][1] = 2;
            jres["clean_robot_pos"][2] = 3;
        }
        else
        {
            jres["clean_robot_pos"][0] = pose_vector.back().pose_x;
            jres["clean_robot_pos"][1] = pose_vector.back().pose_y;
            jres["clean_robot_pos"][2] = pose_vector.back().pose_yam;
        }
       
        
        jres["charge_base_pos"][0] = 0;
        jres["charge_base_pos"][1] = 1;
    }
    if (state_type == "get_robot_traj")
    {
        if (pose_vector.empty())
        {
            return;
        }
        int i = 0;
        for (auto& pose:pose_vector)
        {
            jres["robot_traj"][i][0] = pose.pose_x;
            jres["robot_traj"][i][1] = pose.pose_y;
            jres["robot_traj"][i][2] = pose.pose_yam;
            i++;
        }
        
    }
    if (state_type == "update_clean_trail")
    {
        int JV_TO_INT(jreq[JKEY_BODY], "start_pos", start_pos, 0);
        jres["start_pos"] = start_pos;
        int i = 0;
        for (; start_pos < pose_vector.size(); start_pos++)
        {
            jres["clean_trail"][i][0] = pose_vector[start_pos].pose_x;
            jres["clean_trail"][i][1] = pose_vector[start_pos].pose_y;
            i++;
        }
        if (!jres.isMember("clean_trail"))
        {
            Json::Value trail_null(Json::arrayValue); 
            jres["clean_trail"] = trail_null; 
            return;
        }
        
    }
    

    
}