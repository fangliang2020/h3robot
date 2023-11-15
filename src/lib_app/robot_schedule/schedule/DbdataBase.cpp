#include "DbdataBase.h"
std::mutex DbdataBase::db_instMutex;
DbdataBase *DbdataBase::m_dbdata_base = nullptr;
DbdataBase::DbdataBase()
{
    dp_dbdata_client_ = nullptr;
    Mydbdata_Service_ = nullptr;
    clean_mode = 0;
    reservation_value = 1;
    donot_disturb_value = 1;
}
DbdataBase::~DbdataBase()
{
    if (m_dbdata_base)
    {
        delete m_dbdata_base;
        m_dbdata_base = nullptr;
    }
    
}
DP_MSG_HDR(DbdataBase, DP_MSG_DEAL, pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[] = {
  { "set_clean_type",       &DbdataBase::Clean_type},
  { "set_clean_strength",   &DbdataBase::Clean_strength},
  { "set_timing_clean_cfg", &DbdataBase::Time_clean},
  { "set_donot_disturb_cfg", &DbdataBase::Donot_disturb},
  { "set_self_clean_cfg", &DbdataBase::Self_clean},
  { "set_zone_cfg", &DbdataBase::Zone_cfg},
  { "delete_map", &DbdataBase::Delete_map}
  
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);
DbdataBase *DbdataBase::getDbInstruct()
{
    if (m_dbdata_base == nullptr)
    {
        db_instMutex.lock();
        if (m_dbdata_base == nullptr)
        {
            m_dbdata_base = new DbdataBase();
        }
        db_instMutex.unlock();
    }
    
    return m_dbdata_base;
    
}
void DbdataBase::Clean_type(Json::Value &jreq, Json::Value &jret) //获取清扫模式
{
    auto JV_TO_INT(jreq, "mode", model, 0);
    std::cout<<"the jreq"<< jreq.toSimpleString() <<std::endl;
    std::cout<<"the model"<< model <<std::endl;
    switch (model)
    {
    case 1:   //只扫不拖
        {
        int mid_level = 1,mop_ctrl = 0,side_level = 2; 
        std::cout <<"set_mid_level  1"<<std::endl;
        SensorData::getsensorData()->set_mid_level(mid_level);
        SensorData::getsensorData()->set_mop_ctrl(mop_ctrl);
        SensorData::getsensorData()->set_side_level(side_level);
        break;
        }
    case 2:   //只拖不扫
        {
        int mid_level = 0,mop_ctrl = 1,side_level = 0;
        std::cout <<"set_mid_level 2"<<std::endl;
        SensorData::getsensorData()->set_mid_level(mid_level);
        SensorData::getsensorData()->set_mop_ctrl(mop_ctrl);
        SensorData::getsensorData()->set_side_level(side_level);
        break;
        }
    case 3:  //边扫边拖
        {
        int mid_level = 1,mop_ctrl = 1,side_level = 2;
        std::cout <<"set_mid_level 3"<<std::endl;
        SensorData::getsensorData()->set_mid_level(mid_level);
        SensorData::getsensorData()->set_mop_ctrl(mop_ctrl);
        SensorData::getsensorData()->set_side_level(side_level);   
        break;
        }
    
    default:
        std::cout <<"set_mid_level default"<<std::endl;
        break;
    }
    
}

void DbdataBase::Clean_strength(Json::Value &jreq, Json::Value &jret) //获取清洁模式
{
    auto JV_TO_INT(jreq, "mode", model, 0);
    switch (model)
    {
    case 1:
         //普通清洁
        break;
    case 2: //深度清洁
        break;
    case 3: //自定义清洁
    {
        auto JV_TO_INT(jreq, "suction", suction, 0); //吸力
        auto JV_TO_INT(jreq, "water_yield", water_yield, 0); //水量
        auto JV_TO_INT(jreq, "speed", speed, 0);//转速
        auto JV_TO_INT(jreq, "area", clean_area, 0);//TODO发送回洗面积给算法
        break;
    }
    default:
        break;
    }
   

}
void DbdataBase::Time_clean(Json::Value &jreq, Json::Value &jret) //TODO获取预约清扫
{
    auto JV_TO_INT(jreq, "enable", enable, 0);
    switch (enable)
    {
    case 0:
        reservation_value = 0;
        break;
    case 1:
        reservation_value =1;
        break;
    
    default:
        break;
    }
    // if (enable == 1)
    // {
    //         //auto JV_TO_INT(dpjreq[JKEY_BODY], "enable", enable, 0);
    //         //Json2String(dpjreq, jrep_arry);
    //     reservation_value = 1;
    //     if (jreq["days"].isArray())
    //     {
    //         int jreq_size = jreq["days"].size();
    //         for (int i = 0; i < jreq_size; i++)
    //         {
    //             auto day = jreq["days"][i]["day"].asInt();
    //             auto enable = jreq["days"][i]["enable"].asInt();
    //             auto start_time = jreq["days"][i]["start_time"].asInt();
    //         }
                
    //     }
    // }

}

void DbdataBase::Donot_disturb(Json::Value &jreq, Json::Value &jret)//监听勿扰模式
{
    auto JV_TO_INT(jreq, "enable", enable, 0);
    if (enable == 1)
    {
        // auto JV_TO_INT(jreq, "start_time", start_time, 0);
        // auto JV_TO_INT(jreq, "end_time", end_time, 0);
        donot_disturb_value = 1;
        
    }
    
}
void DbdataBase::Self_clean(Json::Value &jreq, Json::Value &jret) //自清洁模式获取
{
    auto JV_TO_INT(jreq, "enable", enable, 0);
    switch (enable)
    {
    case 0:
        /*只清洁*/
        break;
    case 1:
        /* 集尘和清洁 */
        break;
    default:
        break;
    }
}

void DbdataBase::Zone_cfg(Json::Value &jreq, Json::Value &jret) //区域配置更改监听
{
    // int vertexs_data[4][2] = {0};
    // int virtual_wall[4][2] = {0};
    // int forbidden_zone[4][2] = {0};
    // int threshold_data[4][2] = {0};
    std::vector<Room> room_vertexs;
    std::vector<Room> virtual_wall;
    std::vector<Room> forbidden_zone;
    std::vector<Room> virtual_threshold;
    auto JV_TO_INT(jreq, "map_id", map_id, 0);
    Room zone_cfg;
    if (jreq["room"].isArray()) //卧室
    {
        // if (!room_vertexs.empty())
        // {
        //     //room_vertexs.clear();
        //     vector<Room>().swap(room_vertexs);
        // }
        int room_size = jreq["room"].size();
        for (int i = 0; i < room_size; i++)
        {
            zone_cfg.bedroom_name = jreq["room"][i]["name"].asString();
            zone_cfg.room_id = jreq["room"][i]["id"].asInt();
            if (jreq["room"][i]["vertexs"].isArray())
            {
                int jreq_size = jreq["room"][i]["vertexs"].size();
                for (int j = 0; j < jreq_size; j++)
                {
                    zone_cfg.vertexs_value[j][0] = jreq["room"][i]["vertexs"][j][0].asInt();
                    zone_cfg.vertexs_value[j][1] = jreq["room"][i]["vertexs"][j][1].asInt();//TODO
                }
            
            }
            room_vertexs.emplace_back(zone_cfg);
        }

        set_forbidden_zone(room_vertexs);
        
    }
    if (jreq["virtual_wall"].isArray()) //虚拟墙
    {
        // if (!virtual_wall.empty())
        // {
        //     vector<Room>().swap(virtual_wall);
        // }
        
        int room_size = jreq["virtual_wall"].size();
        for (int i = 0; i < room_size; i++)
        {
            zone_cfg.bedroom_name = jreq["virtual_wall"][i]["name"].asString();
            zone_cfg.room_id = jreq["virtual_wall"][i]["id"].asInt();
            if (jreq["virtual_wall"][i]["vertexs"].isArray())
            {
                int jreq_size = jreq["virtual_wall"][i]["vertexs"].size();
                for (int j = 0; j < jreq_size; j++)
                {
                    zone_cfg.vertexs_value[j][0] = jreq["virtual_wall"][i]["vertexs"][j][0].asInt();
                    zone_cfg.vertexs_value[j][1] = jreq["virtual_wall"][i]["vertexs"][j][1].asInt();//TODO
                }
            
            }
            virtual_wall.emplace_back(zone_cfg);
        }
        set_forbidden_zone(virtual_wall);
    }
    if (jreq["virtual_forbidden_zone"].isArray()) //禁区
    {
        // if (!forbidden_zone.empty())
        // {
        //     vector<Room>().swap(forbidden_zone);
        // }
        
        int room_size = jreq["virtual_forbidden_zone"].size();
        for (int i = 0; i < room_size; i++)
        {
            zone_cfg.bedroom_name = jreq["virtual_forbidden_zone"][i]["name"].asString();
            zone_cfg.room_id = jreq["virtual_forbidden_zone"][i]["id"].asInt();
            if (jreq["virtual_forbidden_zone"][i]["vertexs"].isArray())
            {
                int jreq_size = jreq["virtual_forbidden_zone"][i]["vertexs"].size();
                for (int j = 0; j < jreq_size; j++)
                {
                    zone_cfg.vertexs_value[j][0] = jreq["virtual_forbidden_zone"][i]["vertexs"][j][0].asInt();
                    zone_cfg.vertexs_value[j][1] = jreq["virtual_forbidden_zone"][i]["vertexs"][j][1].asInt();//TODO
                }
            
            }
            forbidden_zone.emplace_back(zone_cfg);
        }
        set_forbidden_zone(forbidden_zone);
    }
   
    if (jreq["virtual_threshold"].isArray()) //门槛条
    {
        // if (!virtual_threshold.empty())
        // {
        //     vector<Room>().swap(forbidden_zone);
        // }
        
        int room_size = jreq["virtual_threshold"].size();
        for (int i = 0; i < room_size; i++)
        {
            zone_cfg.bedroom_name = jreq["virtual_threshold"][i]["name"].asString();
            zone_cfg.room_id = jreq["virtual_threshold"][i]["id"].asInt();
            if (jreq["virtual_threshold"][i]["vertexs"].isArray())
            {
                int jreq_size = jreq["virtual_threshold"][i]["vertexs"].size();
                for (int j = 0; j < jreq_size; j++)
                {
                    zone_cfg.vertexs_value[j][0] = jreq["virtual_threshold"][i]["vertexs"][j][0].asInt();
                    zone_cfg.vertexs_value[j][1] = jreq["virtual_threshold"][i]["vertexs"][j][1].asInt();//TODO
                }
            
            }
            virtual_threshold.emplace_back(zone_cfg);
        }
        set_forbidden_zone(virtual_threshold);
        
    }
    

}
void DbdataBase::Delete_map(Json::Value &jreq, Json::Value &jret) //删除地图
{
    auto JV_TO_INT(jreq, "map_id", map_id, 0);
    switch (map_id)
    {
    case 1:
        /* code */
        system("rm /userdata/map1/* -rf");
        break;
    case 2:
        system("rm /userdata/map2/* -rf");
        break;
    case 3:
        system("rm /userdata/map3/* -rf");
        break;
    
    default:
        break;
    }
}

void DbdataBase::OnDpDb_Message(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg)
{
    std::string stype;
    Json::Value jreq,jret;
    //std::cout <<"OnDpDb_Message"<<std::endl;
    DLOG_DEBUG(MOD_EB,"=== dp %s",dp_msg->req_buffer.c_str());
    if(dp_msg->method=="DP_DB_BROADCAST") 
    {
        if (!String2Json(dp_msg->req_buffer, jreq)) {
        DLOG_ERROR(MOD_EB, "json parse failed.");
        return ;
    }
    stype = jreq[JKEY_TYPE].asString();
    //DbdataBase::Dpwork_handle(stype,jreq);
    for (uint32 idx = 0; idx < g_dpmsg_table_size; ++idx) {
    if (stype == g_dpmsg_table[idx].type) {
       (this->*g_dpmsg_table[idx].handler) (jreq[JKEY_BODY],jret);
      break;
    }
  }

  } 

}
void DbdataBase::save_db_map(int &map_id)//TODO需要根据地图数量修改map1 map2 map3
{
    DLOG_INFO(MOD_BUS, "save_db_map");
    std::string str_req, str_res;
    Json::Value json_req;
    json_req[JKEY_TYPE] = "save_map";
    json_req[JKEY_BODY]["map_id"] = map_id;
    switch (map_id)
    {
    case 1:
        json_req[JKEY_BODY]["map_info_path"] = "/userdata/map/housemap_info.txt";
        json_req[JKEY_BODY]["map_data_path"] = "/userdata/map/housemap.txt";
        break;
    case 2:
        json_req[JKEY_BODY]["map_info_path"] = "/userdata/map2/housemap_info.txt";
        json_req[JKEY_BODY]["map_data_path"] = "/userdata/map2/housemap.txt";
        break;
    case 3:
        json_req[JKEY_BODY]["map_info_path"] = "/userdata/map3/housemap_info.txt";
        json_req[JKEY_BODY]["map_data_path"] = "/userdata/map3/housemap.txt";
        break;
        
    default:
        break;
    }

    Json2String(json_req, str_req);
    dp_dbdata_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

    return;
}
int DbdataBase::get_map_number()
{
    int map1_number = -1,map2_number = -1,map3_number = -1;
    if (access("/userdata/map/housemap_info.txt",F_OK) == 0)
    {
        map1_number = 2;
    }
    if (access("/userdata/map2/housemap_info.txt",F_OK) == 0)
    {
        map2_number = 3;
    }
    if (access("/userdata/map3/housemap_info.txt",F_OK) == 0)
    {
        map3_number = 4;
    }
    switch (map1_number+map2_number+map3_number)
    {
    case -3: //no map
        return 0;
        
    case 0: //have first map
        return 2;
        
    case 1: //have second map
        return 3;
    case 2:   
        return 4;
       
    case 4: //one and two map
        return 5;
       
    case 5: //one and three
        return 6;
       
    case 6://two and three
        return 7;
       
    case 9://one and two and three
        return 9;
        
    default:
        break;
    } 
    
}
int DbdataBase::get_clean_type()
{
    DLOG_INFO(MOD_BUS, "get_clean_type");
    std::string str_req, str_res;
    Json::Value json_req, json_res;
    json_req[JKEY_TYPE] = "get_clean_type";
    Json2String(json_req, str_req);
    dp_dbdata_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
    if(String2Json(str_res, json_res) && json_res[JKEY_BODY]["mode"].isInt())
    {
        clean_mode = json_res[JKEY_BODY]["mode"].asInt();
    }
    else
    {
        DLOG_WARNING(MOD_BUS, "the value is %s",str_res.c_str());
        return 0;
    }
    return clean_mode;
}
int DbdataBase::get_donot_disturb_cfg()
{
    DLOG_INFO(MOD_BUS, "get_donot_disturb_cfg");
    std::string str_req, str_res;
    Json::Value json_req, json_res;
    int enable = -1;
    json_req[JKEY_TYPE] = "get_donot_disturb_cfg";
    Json2String(json_req, str_req);
    dp_dbdata_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
    if(String2Json(str_res, json_res) && json_res[JKEY_BODY]["enable"].isInt())
    {
        enable = json_res[JKEY_BODY]["enable"].asInt();
        disturb_start_time = json_res[JKEY_BODY]["start_time"].asInt();
        disturb_end_time = json_res[JKEY_BODY]["end_time"].asInt();

    }
    else
    {
        DLOG_WARNING(MOD_BUS, "the value is %s",str_res.c_str());
        return enable;
    }
    return enable;
}
int DbdataBase::get_disturb_start_time()
{ 
    return disturb_start_time;
}
int DbdataBase::get_disturb_end_time()
{
    return disturb_end_time;
}
int DbdataBase::get_timing_clean_cfg()
{
    DLOG_INFO(MOD_BUS, "get_timing_clean_cfg");
    std::string str_req, str_res;
    Json::Value json_req, json_res;
    int enable = -1;
    Clean_value m_value;
    json_req[JKEY_TYPE] = "get_timing_clean_cfg";
    Json2String(json_req, str_req);
    dp_dbdata_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
    if (String2Json(str_res, json_res) && json_res[JKEY_BODY]["enable"].isInt())
    {
        enable = json_res[JKEY_BODY]["enable"].asInt();
        if (enable == 0)
        {
            return 0;
        }
        
        if (enable == 1)
        {
            if (json_res[JKEY_BODY]["days"].isArray())
            {
                int jreq_size = json_res[JKEY_BODY]["days"].size();
                for (int i = 0; i < jreq_size; i++)
                {
                    m_value.day = json_res[JKEY_BODY]["days"][i]["day"].asInt();
                    m_value.enable = json_res[JKEY_BODY]["days"][i]["enable"].asInt();
                    m_value.start_time = json_res[JKEY_BODY]["days"][i]["start_time"].asInt();
                    if (m_value.enable == 1)
                    {
                        time_clean.emplace_back(m_value);
                    }
                }
                
            }
            return 1;
        }
        
    }
    
}
void DbdataBase::get_zone_cfg()
{
    DLOG_INFO(MOD_BUS, "get_zone_cfg");
    std::string str_req, str_res;
    Json::Value json_req, json_res;
    std::vector<Room> room_vertexs;
    std::vector<Room> virtual_wall;
    std::vector<Room> forbidden_zone;
    std::vector<Room> virtual_threshold;
    json_req[JKEY_TYPE] = "get_zone_cfg";
    json_req[JKEY_BODY]["map_id"] = 1;
    Json2String(json_req, str_req);
    dp_dbdata_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
    if (String2Json(str_res, json_res) && json_res[JKEY_BODY]["map_id"].isInt())
    {

        auto map_id = json_res[JKEY_BODY]["map_id"].asInt();
        Room m_vertexs;
        if (json_res[JKEY_BODY]["room"].isArray()) //卧室
        {
            // if (!room_vertexs.empty())
            // {
            //     vector<Room>().swap(room_vertexs);
            // }
            int room_size = json_res[JKEY_BODY]["room"].size();
            for (int i = 0; i < room_size; i++)
            {
                m_vertexs.bedroom_name = json_res[JKEY_BODY]["room"][i]["name"].asString(); //TODO
                m_vertexs.room_id = json_res[JKEY_BODY]["room"][i]["id"].asInt();//TODO
                if (json_res[JKEY_BODY]["room"][i]["vertexs"].isArray())
                {
                    int jreq_size = json_res[JKEY_BODY]["room"][i]["vertexs"].size();
                    for (int j = 0; j < jreq_size; j++)
                    {
                        m_vertexs.vertexs_value[j][0] = json_res[JKEY_BODY]["room"][i]["vertexs"][j][0].asInt();
                        m_vertexs.vertexs_value[j][1] = json_res[JKEY_BODY]["room"][i]["vertexs"][j][1].asInt();//TODO
                        
                    }
                    
                }
                room_vertexs.emplace_back(m_vertexs);
            }
            set_forbidden_zone(room_vertexs);
        
        }
        if (json_res[JKEY_BODY]["virtual_wall"].isArray()) //虚拟墙
        {
            // if (!virtual_wall.empty())
            // {
            //     vector<Room>().swap(virtual_wall);
            // }
            int virtual_size = json_res[JKEY_BODY]["virtual_wall"].size();
            for (int i = 0; i < virtual_size; i++)
            {
                m_vertexs.bedroom_name = json_res[JKEY_BODY]["virtual_wall"][i]["name"].asString(); //TODO
                m_vertexs.room_id = json_res[JKEY_BODY]["virtual_wall"][i]["id"].asInt();//TODO
                if (json_res[JKEY_BODY]["virtual_wall"][i]["vertexs"].isArray())
                {
                    int jreq_size = json_res[JKEY_BODY]["virtual_wall"][i]["vertexs"].size();
                    for (int j = 0; j < jreq_size; j++)
                    {
                        m_vertexs.vertexs_value[j][0] = json_res[JKEY_BODY]["virtual_wall"][i]["vertexs"][j][0].asInt();
                        m_vertexs.vertexs_value[j][1] = json_res[JKEY_BODY]["virtual_wall"][i]["vertexs"][j][1].asInt();//TODO
                        
                    }
                    
                }
                virtual_wall.emplace_back(m_vertexs);
            }
            set_forbidden_zone(virtual_wall);
        }
        if (json_res[JKEY_BODY]["virtual_forbidden_zone"].isArray()) //禁区
        {
            // if (!forbidden_zone.empty())
            // {
            //     vector<Room>().swap(forbidden_zone);
            // }
            int forbidden_size = json_res[JKEY_BODY]["room"].size();
            for (int i = 0; i < forbidden_size; i++)
            {
                m_vertexs.bedroom_name = json_res[JKEY_BODY]["virtual_forbidden_zone"][i]["name"].asString(); //TODO
                m_vertexs.room_id = json_res[JKEY_BODY]["virtual_forbidden_zone"][i]["id"].asInt();//TODO
                if (json_res[JKEY_BODY]["virtual_forbidden_zone"][i]["vertexs"].isArray())
                {
                    int jreq_size = json_res[JKEY_BODY]["virtual_forbidden_zone"][i]["vertexs"].size();
                    for (int j = 0; j < jreq_size; j++)
                    {
                        m_vertexs.vertexs_value[j][0] = json_res[JKEY_BODY]["virtual_forbidden_zone"][i]["vertexs"][j][0].asInt();
                        m_vertexs.vertexs_value[j][1] = json_res[JKEY_BODY]["virtual_forbidden_zone"][i]["vertexs"][j][1].asInt();//TODO
                        
                    }
                    
                }
                forbidden_zone.emplace_back(m_vertexs);
            }
            set_forbidden_zone(forbidden_zone);
        }
        if (json_res[JKEY_BODY]["virtual_threshold"].isArray()) //门槛条
        {
            // if (!virtual_threshold.empty())
            // {
            //     vector<Room>().swap(virtual_threshold);
            // }
            int threshold_size = json_res[JKEY_BODY]["virtual_threshold"].size();
            for (int i = 0; i < threshold_size; i++)
            {
                m_vertexs.bedroom_name = json_res[JKEY_BODY]["virtual_threshold"][i]["name"].asString(); //TODO
                m_vertexs.room_id = json_res[JKEY_BODY]["virtual_threshold"][i]["id"].asInt();//TODO
                if (json_res[JKEY_BODY]["virtual_threshold"][i]["vertexs"].isArray())
                {
                    int jreq_size = json_res[JKEY_BODY]["virtual_threshold"][i]["vertexs"].size();
                    for (int j = 0; j < jreq_size; j++)
                    {
                        m_vertexs.vertexs_value[j][0] = json_res[JKEY_BODY]["virtual_threshold"][i]["vertexs"][j][0].asInt();
                        m_vertexs.vertexs_value[j][1] = json_res[JKEY_BODY]["virtual_threshold"][i]["vertexs"][j][1].asInt();//TODO
                        
                    }
                    
                }
                virtual_threshold.emplace_back(m_vertexs);
            }
            set_forbidden_zone(virtual_threshold);
        }
        
    }
    

}
void DbdataBase::work_clean_strength()
{
    DLOG_INFO(MOD_BUS, "get_clean_strength");
    std::string str_req, str_res;
    Json::Value json_req, json_res;
    json_req[JKEY_TYPE] = "get_clean_strength";
    Json2String(json_req, str_req);
    dp_dbdata_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
    if(String2Json(str_res, json_res) && json_res[JKEY_BODY]["mode"].isInt())
    {
        auto strength_mode = json_res[JKEY_BODY]["mode"].asInt();
        switch (strength_mode)
        {
        case 1:
        {
            int ordinary = 1;
            SensorData::getsensorData()->set_mid_level(ordinary);
            SensorData::getsensorData()->set_side_level(ordinary);
            //SensorData::getsensorData()->set_fan_level(peripheral);
            break;
        }
        case 2:
        {
            int depth = 3;
            SensorData::getsensorData()->set_mid_level(depth);
            SensorData::getsensorData()->set_side_level(depth);
            //SensorData::getsensorData()->set_fan_level(peripheral);
            break;
        }
        case 3:
        {
            auto JV_TO_INT(json_res, "suction", suction, 0); //吸力
            auto JV_TO_INT(json_res, "water_yield", water_yield, 0); //水量
            auto JV_TO_INT(json_res, "speed", speed, 0);//转速
            auto JV_TO_INT(json_res, "area", clean_area, 0);//TODO发送回洗面积给算法
            SensorData::getsensorData()->set_fan_level(suction);
            break;
        } 
        default:
            break;
        }

    }
    else
    {
        DLOG_WARNING(MOD_BUS, "the value is %s",str_res.c_str());
        return;
    }
    return;
}
void DbdataBase::work_clean_type()
{
    int clean_type = get_clean_type();
    switch (clean_type)
    {
    case 1:   //只扫不拖
        {
        int mid_level = 1,mop_ctrl = 0,side_level = 2; 
        std::cout <<"set_mid_level  1"<<std::endl;
        SensorData::getsensorData()->set_side_level(side_level);
        SensorData::getsensorData()->set_mid_level(mid_level);
        SensorData::getsensorData()->set_mop_ctrl(mop_ctrl);
        break;
        }
    case 2:   //只拖不扫
        {
        int mid_level = 0,mop_ctrl = 1,side_level = 0;
        std::cout <<"set_mid_level 2"<<std::endl;
        SensorData::getsensorData()->set_side_level(side_level);
        SensorData::getsensorData()->set_mid_level(mid_level);
        SensorData::getsensorData()->set_mop_ctrl(mop_ctrl);
        break;
        }
    case 3:  //边扫边拖
        {
        int mid_level = 1,mop_ctrl = 1,side_level = 2;
        std::cout <<"set_mid_level 3"<<std::endl;
        SensorData::getsensorData()->set_side_level(side_level);
        SensorData::getsensorData()->set_mid_level(mid_level);
        SensorData::getsensorData()->set_mop_ctrl(mop_ctrl);   
        break;
        }
    
    default:
        break;
    }
    return;
}
void DbdataBase::saveAllmaponDb()
{
    auto map_number = get_map_number();
    switch (map_number)
    {
    case 2:
        {
        auto real_mapId = map_number -1;
        save_db_map(real_mapId);
        break;
        }
    case 3:
        {
        auto real_mapId = map_number -1;
        save_db_map(real_mapId);
        break;
        }
    case 4:
        {
        auto real_mapId = map_number -1;
        save_db_map(real_mapId);
        break;
        }
    case 5:
        {
        auto real_onemapId = map_number -4;
        auto real_twomapId = map_number -3;
        save_db_map(real_onemapId);
        save_db_map(real_twomapId);
        break;
        }
    case 6:
        {
        auto real_onemapId = map_number -5;
        auto real_threemapId = map_number -3;
        save_db_map(real_onemapId);
        save_db_map(real_threemapId);
        break;
        }
    case 7:
        {
        auto real_twomapId = map_number -5;
        auto real_threemapId = map_number -4;
        save_db_map(real_twomapId);
        save_db_map(real_threemapId);
        break;
        }
    case 9:
        {
        auto real_onemapId = map_number - 8;
        auto real_twomapId = map_number - 7;
        auto real_threemapId = map_number - 6;
        save_db_map(real_onemapId);
        save_db_map(real_twomapId);
        save_db_map(real_threemapId);
        break;
        }
    default:
        break;
    }
    return;
}
void DbdataBase::dbCfgtime()
{
    while(true)
	{
		db_timer.DetectTimers();
		usleep(50*1000);
        //std::cout << "------------------------"<< std::endl;
         if (CRobotSchedule::getRobotSchedule()->getCRobotdata() == false)
         {
             break;
         }
        
	}
}
void DbdataBase::dbCfgwork()
{
    if (donot_disturb_value == 1)
    {
        donot_disturb_value = 0;
        int disturb_enable = get_donot_disturb_cfg();
        if (disturb_enable == 1)
        {
            DayTime disturb_time;
            int start_donot_work_time = get_disturb_start_time();
            int end_donot_work_time = get_disturb_end_time();
            CDataModel::getDataModel()->getCurrentTime();
            int now_real_time = disturb_time.time_hour*60 + disturb_time.time_min;
            if (start_donot_work_time < now_real_time < end_donot_work_time)
            {
                auto now_device_state = CDataModel::getDataModel()->getMState();
                if (now_device_state != ESTATE_IDLE)
                {
                    interrupt_work();
                    
                }
            
            }
            

        }
    }
    
    
    

    if (reservation_value != 0)
    {
        
        reservation_value = 0;
        get_timing_clean_cfg();
        if (!time_clean.empty())
        {
                DayTime now_time;
                for (int i = 0; i < time_clean.size(); i++)
                {
                    auto day = time_clean[i].day;
                    auto start_time = time_clean[i].start_time;
                    CDataModel::getDataModel()->getCurrentTime();
                    int now_time_division = now_time.time_hour*60 + now_time.time_min;
                    if(now_time.time_wday == day && now_time_division == start_time)
                    {
                        auto now_work_state = CDataModel::getDataModel()->getMState();
                        if (now_work_state == ESTATE_CHAGRE || now_work_state == ESTATE_CHAGRE_END)
                        {
                            CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_GLOBAL_CLEAN);
                        }
                        
                    }

                }
                vector<Clean_value>().swap(time_clean);
            
        }
       

        
    }

}
chml::DpClient::Ptr DbdataBase::GetDpdbBaseClientPtr()
{
    return dp_dbdata_client_;
}
bool DbdataBase::init()
{
    std::cout << "DbdataBase init start" <<std::endl;
    Mydbdata_Service_ = chml::EventService::CreateEventService(NULL, "Mydbdata_Service");
    chml::SocketAddress dp_per_addr(ML_DP_SERVER_HOST, ML_DP_SERVER_PORT);
    dp_dbdata_client_=chml::DpClient::CreateDpClient(Mydbdata_Service_,dp_per_addr,"dp_dbdata_service");
    ASSERT_RETURN_FAILURE(!dp_dbdata_client_, false);
    dp_dbdata_client_->SignalDpMessage.connect(this,&DbdataBase::OnDpDb_Message);
    //接收到消息后需要回复request,相当于注册一个 "DP_PERIPHERALS_REQUEST"服务，都可以被其他进程调用
    if (dp_dbdata_client_->ListenMessage("DP_DB_BROADCAST", chml::TYPE_MESSAGE)) {
    DLOG_ERROR(MOD_EB, "dp client listen message failed.");
     }
    db_timerTh = new std::thread(std::bind(&DbdataBase::dbCfgtime,this));
    db_timer.register_timer([](uint64_t){DbdataBase::getDbInstruct()->dbCfgwork();}, 10000, 1000, REPEAT_FOREVER);
    sleep(1);
    std::cout << "DbdataBase init success" <<std::endl;
    return true;
}