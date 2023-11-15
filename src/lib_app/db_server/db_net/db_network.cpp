#include "db_network.h"
#include <glib.h>
#include "json-c/json.h"
#include "mldb.h"
#include "db_server/base/common.h"
#include "db_server/main/db_manager.h"
#include "db_server/base/jkey_common.h"

#define TABLE_NETWORK_SERVICE       (char*)"NetworkService"
#define TABLE_NETWORK_NTP           (char*)"ntp"
#define TABLE_NETWORK_SN            (char*)"device_sn"
#define TABLE_NETWORK_VERSION       (char*)"NetworkVersion"
#define TABLE_NETWORK_BKURL         (char*)"BackgroudUrl"
#define TABLE_NETWORK_MAPPATH       (char*)"MapPath"
#define TABLE_NETWORK_CLEANTYPE     (char*)"CleanType"
#define TABLE_NETWORK_CLEANSTRENGTH (char*)"CleanStrength"
#define TABLE_NETWORK_NODISTURB     (char*)"NoDisturb"
#define TABLE_NETWORK_TIMINGCLEANONOFF   (char*)"TimingCleanOnOff"
#define TABLE_NETWORK_TIMINGCLEANCFG   (char*)"TimingCleanCfg"
#define TABLE_NETWORK_SELFCLEAN     (char*)"SelfClean"
#define TABLE_NETWORK_ZONECFG       (char*)"ZoneCfg"
#define TABLE_NETWORK_ACTIVE        (char*)"Active"
#define NETWORK_VERSION             (char*)"1.0.0"

namespace db{

DP_MSG_HDR(Db_Network,DP_MSG_DEAL,pFUNC_DealDpMsg)
static DP_MSG_DEAL g_dpmsg_table[]={
    {"get_sn",                &Db_Network::Get_SN},
    {"set_clean_type",        &Db_Network::Set_Clean_Type},
    {"get_clean_type",        &Db_Network::Get_Clean_Type},
    {"set_clean_strength",    &Db_Network::Set_Clean_Strength},
    {"get_clean_strength",    &Db_Network::Get_Clean_Strength},
    {"set_donot_disturb_cfg", &Db_Network::Set_Donot_Disturb_Cfg},
    {"get_donot_disturb_cfg", &Db_Network::Get_Donot_Disturb_Cfg},
    {"set_timing_clean_cfg",  &Db_Network::Set_Timing_Clean_Cfg},
    {"get_timing_clean_cfg",  &Db_Network::Get_Timing_Clean_Cfg},
    {"set_self_clean_cfg",    &Db_Network::Set_Self_Clean_Cfg},
    {"get_self_clean_cfg",    &Db_Network::Get_Self_Clean_Cfg},  
    {"set_zone_cfg",          &Db_Network::Set_Zone_Cfg},
    {"get_zone_cfg",          &Db_Network::Get_Zone_Cfg},
    {"save_map",              &Db_Network::Save_Map},
    {"delete_map",            &Db_Network::Delete_Map},
    {"query_map",             &Db_Network::Query_Map},  //取地图和Save_Map是在一个table中的
    {"edit_map_name",         &Db_Network::Edit_MapName},
    {"set_activate",          &Db_Network::Set_Active},
    {"get_activate",          &Db_Network::Get_Active},
    {"set_wifi",              &Db_Network::Set_Wifi},
    {"get_wifi",              &Db_Network::Get_Wifi},
};

static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

Db_Network::Db_Network(DbServer *db_server_ptr,YamlConfig *YamlConfig)
    : BaseModule(db_server_ptr),dp_client_(nullptr),YamlPtr_(YamlConfig) {
    dp_client_ = Dbptr_->GetDpClientPtr();
}
Db_Network::~Db_Network() {
   
}  

int Db_Network::Start() {
    Network_DbInit(YamlPtr_);
    // Json::Value jreq;
    // Json::Value jret;
    // Get_SN(jreq,jret);
    return 0;
}

int Db_Network::Stop() {
    return 0;
}

int Db_Network::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
    DLOG_DEBUG(MOD_EB, "OnDpMessage:%s.", stype.c_str());
    int nret = -1;
    for (uint32 idx = 0; idx < g_dpmsg_table_size; ++idx) {
        if (stype == g_dpmsg_table[idx].type) {
        nret = (this->*g_dpmsg_table[idx].handler) (jreq, jret["body"]);
        break;
        }
    }
    return nret;   
}
int Db_Network::Get_SN(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_NETWORK_SN, NULL, (char*)"id=1", NULL, NULL);
    if(String2Json(json_str,jtemp)) {
        jret["sn"] = jtemp["jData"][0]["sn"].asString();
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0;
}
int Db_Network::Set_Clean_Type(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq["body"],"mode",mode,0);
    char *tempbuf=(char *)malloc(50);
    memset(tempbuf,0,50);
    sprintf(tempbuf,"mode=%d",mode);
printf("%s",tempbuf); 
    g_free(mldb_update(TABLE_NETWORK_CLEANTYPE,tempbuf,(char*)"id=1"));  
    free(tempbuf);

    std::string str_req;
    Json2String(jreq, str_req);
    dp_client_->SendDpMessage("DP_DB_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
    return 0;
}
int Db_Network::Get_Clean_Type(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_NETWORK_CLEANTYPE, NULL, (char*)"id=1", NULL, NULL);
printf("%s\n",json_str.c_str());
    if(String2Json(json_str,jtemp)&&jtemp["jData"].size()) {
        jret["mode"] =std::stoi(jtemp["jData"][0]["mode"].asString());
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0;   
}
int Db_Network::Set_Clean_Strength(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq["body"],"mode",mode,0);
    int JV_TO_INT(jreq["body"],"suction",suction,0);
    int JV_TO_INT(jreq["body"],"water_yield",water_yield,0);
    int JV_TO_INT(jreq["body"],"speed",speed,0);
    int JV_TO_INT(jreq["body"],"area",area,0);

    char *tempbuf=(char *)malloc(200);
    memset(tempbuf,0,200);
    sprintf(tempbuf,"mode=%d,suction=%d,water_yield=%d,speed=%d,area=%d",mode,suction,water_yield,speed,area);
printf("%s",tempbuf); 
    g_free(mldb_update(TABLE_NETWORK_CLEANSTRENGTH,tempbuf,(char*)"id=1"));  
    free(tempbuf);

    std::string str_req;
    Json2String(jreq, str_req);
    dp_client_->SendDpMessage("DP_DB_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
    return 0;
}
int Db_Network::Get_Clean_Strength(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_NETWORK_CLEANSTRENGTH, NULL,(char*)"id=1", NULL, NULL);
printf("%s\n",json_str.c_str());
    if(String2Json(json_str,jtemp)&&jtemp["jData"].size()) {
        jret["mode"] = std::stoi(jtemp["jData"][0]["mode"].asString());
        jret["suction"] = std::stoi(jtemp["jData"][0]["suction"].asString());
        jret["water_yield"] = std::stoi(jtemp["jData"][0]["water_yield"].asString());
        jret["speed"]= std::stoi(jtemp["jData"][0]["speed"].asString());
        jret["area"] = std::stoi(jtemp["jData"][0]["area"].asString());
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0; 
}
int Db_Network::Set_Donot_Disturb_Cfg(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq["body"],"enable",enable,0);
    int JV_TO_INT(jreq["body"],"start_time",start_time,0);
    int JV_TO_INT(jreq["body"],"end_time",end_time,0);

    char *tempbuf=(char *)malloc(200);
    memset(tempbuf,0,200);
    sprintf(tempbuf,"enable=%d,start_time=%d,end_time=%d",enable,start_time,end_time);
printf("%s",tempbuf); 
    g_free(mldb_update(TABLE_NETWORK_NODISTURB,tempbuf,(char*)"id=1"));  
    free(tempbuf);

    std::string str_req;
    Json2String(jreq, str_req);
    dp_client_->SendDpMessage("DP_DB_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
    return 0;    
}
int Db_Network::Get_Donot_Disturb_Cfg(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_NETWORK_NODISTURB, NULL,(char*)"id=1", NULL, NULL);
printf("%s\n",json_str.c_str());
    if(String2Json(json_str,jtemp)&&jtemp["jData"].size()) {
        jret["enable"] = std::stoi(jtemp["jData"][0]["enable"].asString());
        jret["start_time"] = std::stoi(jtemp["jData"][0]["start_time"].asString());
        jret["end_time"] = std::stoi(jtemp["jData"][0]["end_time"].asString());
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0; 
}
int Db_Network::Set_Timing_Clean_Cfg(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq["body"],"mode",mode,0);
    int JV_TO_INT(jreq["body"],"enable",enable,0);  
    char *tempbuf=(char *)malloc(200); 
    memset(tempbuf,0,200);
    sprintf(tempbuf,"mode=%d,enable=%d",mode,enable);
printf("%s",tempbuf); 
    //设置开关
    g_free(mldb_update(TABLE_NETWORK_TIMINGCLEANONOFF,tempbuf,(char*)"id=1"));  
    //设置每个星期
    Json::Value jtemp=jreq["body"]["days"];
    std::string id="id=";
    std::string id_temp;
    int maxid=jtemp.size();
    for(int i=0;i<maxid;i++) {
        id_temp=id+std::to_string(i+1);
        int JV_TO_INT(jtemp[i],"day",day,0);
        int JV_TO_INT(jtemp[i],"enable",enable,0); 
        int JV_TO_INT(jtemp[i],"start_time",start_time,0);  
        memset(tempbuf,0,200);
        sprintf(tempbuf,"day=%d,enable=%d,start_time=%d",day,enable,start_time); 
        g_free(mldb_update(TABLE_NETWORK_TIMINGCLEANCFG,tempbuf,(char *)id_temp.c_str())); 
    }
    free(tempbuf);
    std::string str_req;
    Json2String(jreq, str_req);
    dp_client_->SendDpMessage("DP_DB_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
    return 0;
}
int Db_Network::Get_Timing_Clean_Cfg(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp_onoff;
    std::string json_onoff = mldb_select(TABLE_NETWORK_TIMINGCLEANONOFF, NULL, (char*)"id=1", NULL, NULL);
    if(String2Json(json_onoff,jtemp_onoff)&&jtemp_onoff["jData"].size()) {
        jret["mode"] = std::stoi(jtemp_onoff["jData"][0]["mode"].asString());
        jret["enable"] = std::stoi(jtemp_onoff["jData"][0]["enable"].asString());
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    Json::Value jtemp;
    std::string id="id=";
    std::string id_temp;
    for(int i=0;i<7;i++) {
        id_temp=id+std::to_string(i+1);
        std::string json_str = mldb_select(TABLE_NETWORK_TIMINGCLEANCFG, NULL,(char *)id_temp.c_str(), NULL, NULL);
        //  printf("jret : %s\n", json_str.c_str());
        if(String2Json(json_str,jtemp)&&jtemp["jData"].size()) {
            jret["days"][i]["day"] = std::stoi(jtemp["jData"][0]["day"].asString()); //0表示jData里面第一个数组
            jret["days"][i]["enable"]  = std::stoi(jtemp["jData"][0]["enable"].asString());
            jret["days"][i]["start_time"]  = std::stoi(jtemp["jData"][0]["start_time"].asString());
            //printf("jret : %s\n", jret.toStyledString().c_str());
        }
        else {
            return 2; 
        }
    }
printf("jret : %s\n", jret.toStyledString().c_str());
    return 0; 
}
int Db_Network::Set_Self_Clean_Cfg(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq["body"],"enable",enable,0);
    char *tempbuf=(char *)malloc(50);
    memset(tempbuf,0,50);
    sprintf(tempbuf,"enable=%d",enable);
printf("%s",tempbuf); 
    g_free(mldb_update(TABLE_NETWORK_SELFCLEAN,tempbuf,(char*)"id=1"));  
    free(tempbuf);

    std::string str_req;
    Json2String(jreq, str_req);
    dp_client_->SendDpMessage("DP_DB_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
    return 0;  
}
int Db_Network::Get_Self_Clean_Cfg(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_NETWORK_SELFCLEAN, NULL,(char*)"id=1", NULL, NULL);
printf("%s\n",json_str.c_str());
    if(String2Json(json_str,jtemp)&&jtemp["jData"].size()) {
        jret["enable"] = std::stoi(jtemp["jData"][0]["enable"].asString());
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0; 
}
int Db_Network::Set_Zone_Cfg(Json::Value &jreq, Json::Value &jret) {
    //每次设置时，需要根据mapid号先删除对应的条目，再进行插入
    int JV_TO_INT(jreq["body"],"map_id",map_id,0);
    char *tempbuf=(char *)malloc(2000);
    memset(tempbuf,0,2000);
    sprintf(tempbuf,"map_id=%d",map_id);
    g_free(mldb_delete(TABLE_NETWORK_ZONECFG,tempbuf));

    std::string room = jreq["body"]["room"].toSimpleString(); //json转string 或者Json2String(jreq["room"], room);
    std::string wall = jreq["body"]["virtual_wall"].toSimpleString();
    std::string forbidden_zone = jreq["body"]["virtual_forbidden_zone"].toSimpleString();
    std::string threshold = jreq["body"]["virtual_threshold"].toSimpleString();

    memset(tempbuf,0,2000);
    sprintf(tempbuf,"%d,'%s','%s','%s','%s'",map_id,room.c_str(),wall.c_str(),forbidden_zone.c_str(),threshold.c_str());
    g_free(mldb_insert(TABLE_NETWORK_ZONECFG,(char*)"map_id,room,virtual_wall,virtual_forbidden_zone,virtual_threshold",tempbuf));
    free(tempbuf);
    std::string str_req;
    Json2String(jreq, str_req);
    dp_client_->SendDpMessage("DP_DB_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
    return 0;
}
int Db_Network::Get_Zone_Cfg(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq["body"],"map_id",map_id,0);
    Json::Value jtemp,jtemp2;
    std::string id="map_id=";
    std::string id_temp=id+std::to_string(map_id);
    std::string json_str = mldb_select(TABLE_NETWORK_ZONECFG, NULL,(char*)id_temp.c_str(), NULL, NULL);
printf("%s\n",json_str.c_str());
    if(String2Json(json_str,jtemp)&&jtemp["jData"].size()) {
        jret["map_id"] =map_id;
        String2Json(jtemp["jData"][0]["room"].asString(),jtemp2);
        jret["room"] = jtemp2;
        jtemp2.clear();
        String2Json(jtemp["jData"][0]["virtual_wall"].asString(),jtemp2);
        jret["virtual_wall"] = jtemp2;
        jtemp2.clear();
        String2Json(jtemp["jData"][0]["virtual_forbidden_zone"].asString(),jtemp2);
        jret["virtual_forbidden_zone"] = jtemp2;
        jtemp2.clear();
        String2Json(jtemp["jData"][0]["virtual_threshold"].asString(),jtemp2);
        jret["virtual_threshold"] = jtemp2;
        jtemp2.clear();
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        Json::Value arr_test(Json::arrayValue); 
        jret["map_id"] =map_id;
        jret["room"] = arr_test;
        jret["virtual_wall"] = arr_test;
        jret["virtual_forbidden_zone"] = arr_test;
        jret["virtual_threshold"] = arr_test;
    } 
    return 0;
}
int Db_Network::Save_Map(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq["body"],"map_id",map_id,0);
    std::string JV_TO_STRING(jreq["body"],"map_info_path",map_info_path,"");
    std::string JV_TO_STRING(jreq["body"],"map_data_path",map_data_path,"");

    char *tempbuf=(char *)malloc(200);
    memset(tempbuf,0,200);
    sprintf(tempbuf,"%d,'%s','%s','%s'",map_id,"",map_info_path.c_str(),map_data_path.c_str());
    printf("%s",tempbuf);
    g_free(mldb_insert(TABLE_NETWORK_MAPPATH,(char*)"map_id,map_name,map_info_path,map_data_path",tempbuf));
    //循环更新最多保存3个******
    std::string str_req;
    Json2String(jreq, str_req);
    dp_client_->SendDpMessage("DP_DB_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
    return 0;  
}
int Db_Network::Delete_Map(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq["body"],"map_id",map_id,0);  
    std::string id="map_id=";
    std::string id_temp=id+std::to_string(map_id);
    g_free(mldb_delete(TABLE_NETWORK_MAPPATH,(char*)id_temp.c_str()));
    return 0;
}
//查询已保存了哪些地图
int Db_Network::Query_Map(Json::Value &jreq, Json::Value &jret) { 
    int i=0;
    std::string id="id=";
    std::string id_temp;
    Json::Value jtemp;
    while(1) {
        id_temp=id+std::to_string(i+1);
        std::string json_str = mldb_select(TABLE_NETWORK_MAPPATH, NULL,(char *)id_temp.c_str(), NULL, NULL);
        printf("jret : %s,i=%d\n", json_str.c_str(),i);
        if(String2Json(json_str,jtemp)&&jtemp["jData"].size()) {
            jret["map"][i]["map_id"] = std::stoi(jtemp["jData"][0]["map_id"].asString()); //0表示jData里面第一个数组
            jret["map"][i]["map_name"]       = jtemp["jData"][0]["map_name"].asString();
            jret["map"][i]["map_info_path"]  = jtemp["jData"][0]["map_info_path"].asString();
            jret["map"][i]["map_data_path"]  = jtemp["jData"][0]["map_data_path"].asString();
        }
        else  break;
        i++;
    }
    if (!jret.isMember("map")) {
        Json::Value arr_test(Json::arrayValue); 
        jret["map"] = arr_test; 
    }
printf("jret : %s\n", jret.toStyledString().c_str());
    return 0;
}
int Db_Network::Edit_MapName(Json::Value &jreq, Json::Value &jret) { 
    int JV_TO_INT(jreq["body"],"map_id",map_id,0);
    std::string JV_TO_STRING(jreq["body"],"map_name",map_name,"");
    char *tempbuf=(char *)malloc(50);
    char *where=(char *)malloc(20);
    memset(tempbuf,0,50);
    memset(where,0,20);
    sprintf(tempbuf,"map_name='%s'",map_name.c_str());
printf("%s",tempbuf); 
    sprintf(where,"map_id='%d'",map_id);
    g_free(mldb_update(TABLE_NETWORK_MAPPATH,tempbuf,where));  
    free(tempbuf);
    free(where);
    return 0;
}
int Db_Network::Set_Active(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq["body"],"activate",activate,0);
    char *tempbuf=(char *)malloc(50);
    memset(tempbuf,0,50);
    sprintf(tempbuf,"activate=%d",activate);
printf("%s",tempbuf); 
    g_free(mldb_update(TABLE_NETWORK_ACTIVE,tempbuf,(char*)"id=1"));  
    free(tempbuf);

    std::string str_req;
    Json2String(jreq, str_req);
    dp_client_->SendDpMessage("DP_DB_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
    return 0;   
}
int Db_Network::Get_Active(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_NETWORK_ACTIVE, NULL,(char*)"id=1", NULL, NULL);
printf("%s\n",json_str.c_str());
    if(String2Json(json_str,jtemp)&&jtemp["jData"].size()) {
        jret["activate"] = std::stoi(jtemp["jData"][0]["activate"].asString());
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0; 
}
int Db_Network::Set_Wifi(Json::Value &jreq, Json::Value &jret) {
    std::string JV_TO_STRING(jreq["body"],"ssid",ssid,"");
    std::string JV_TO_STRING(jreq["body"],"password",password,"");
    char *tempbuf=(char *)malloc(100);
    memset(tempbuf,0,100);
    sprintf(tempbuf,"ssid=%s,password=%s",ssid.c_str(),password.c_str());
printf("%s",tempbuf); 
    g_free(mldb_update(TABLE_NETWORK_SERVICE,tempbuf,(char*)"id=1"));  
    free(tempbuf);

    std::string str_req;
    Json2String(jreq, str_req);
    dp_client_->SendDpMessage("DP_DB_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
    return 0;  
}
int Db_Network::Get_Wifi(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_NETWORK_SERVICE, NULL,(char*)"id=1", NULL, NULL);
printf("%s\n",json_str.c_str());
    if(String2Json(json_str,jtemp)&&jtemp["jData"].size()) {
        jret["ssid"] = jtemp["jData"][0]["ssid"].asString();
        jret["password"] = jtemp["jData"][0]["password"].asString();
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0; 
}
void Db_Network::Network_DbInit(YamlConfig *YamlConf)
{
    
    char *col_para;

    // if (equal_version(TABLE_NETWORK_VERSION,(char*) NETWORK_VERSION))
    //     return;

    g_free(mldb_drop(TABLE_NETWORK_MAPPATH));
    g_free(mldb_drop(TABLE_NETWORK_CLEANTYPE));
    g_free(mldb_drop(TABLE_NETWORK_CLEANSTRENGTH));
    g_free(mldb_drop(TABLE_NETWORK_NODISTURB));
    g_free(mldb_drop(TABLE_NETWORK_TIMINGCLEANONOFF));
    g_free(mldb_drop(TABLE_NETWORK_SELFCLEAN));
    g_free(mldb_drop(TABLE_NETWORK_ZONECFG));
    g_free(mldb_drop(TABLE_NETWORK_SERVICE));
    g_free(mldb_drop(TABLE_NETWORK_NTP));
    g_free(mldb_drop(TABLE_NETWORK_SN));
    g_free(mldb_drop(TABLE_NETWORK_BKURL));
    g_free(mldb_drop(TABLE_NETWORK_VERSION));

    creat_version_table(TABLE_NETWORK_VERSION, (char*)NETWORK_VERSION);

   //1.设定 NetworkService
    col_para = (char*)"id INTEGER PRIMARY KEY AUTOINCREMENT," \
               "sService TEXT NOT NULL UNIQUE," \
               "sPassword TEXT DEFAULT ''," \
               "iFavorite INT DEFAULT 0," \
               "iAutoconnect INT DEFAULT 0";

    g_free(mldb_create((char*)TABLE_NETWORK_SERVICE, col_para));
    YamlValue NetworkService =YamlConf->root().getSub("NetworkService");
    char *databuf=(char *)malloc(200);
    sprintf(databuf,"'%s','%s',%d,%d", NetworkService.getString("sService").c_str(),
                                           NetworkService.getString("sPassword").c_str(),
                                           NetworkService.getInt("iFavorite"),
                                           NetworkService.getInt("iAutoconnect"));
    //printf("%s",databuf);
    g_free(mldb_insert((char*)TABLE_NETWORK_SERVICE, (char*)"sService,sPassword,iFavorite,iAutoconnect", databuf));
  
    //3.设定 TABLE_NETWORK_SN
    col_para =(char*) "id INTEGER PRIMARY KEY," \
                     "sn TEXT NOT NULL";
    g_free(mldb_create(TABLE_NETWORK_SN, col_para));
    YamlValue sn =YamlConf->root().getSub("device_sn");
    memset(databuf,0,200);
    sprintf(databuf,"'%s'",sn.getString("sn").c_str());

    //printf("%s",databuf);
    g_free(mldb_insert(TABLE_NETWORK_SN,(char*)"sn",databuf));

    //4.设定 TABLE_NETWORK_NTP
    col_para =(char*) "id INTEGER PRIMARY KEY," \
               "sNtpServers TEXT NOT NULL," \
               "sTimeZone TEXT NOT NULL," \
               "sTimeZoneFile TEXT NOT NULL," \
               "sTimeZoneFileDst TEXT NOT NULL," \
               "iAutoMode INT DEFAULT 0," \
               "iAutoDst INT DEFAULT 0," \
               "iRefreshTime INT DEFAULT 120";
    g_free(mldb_create(TABLE_NETWORK_NTP, col_para));
    YamlValue ntp =YamlConf->root().getSub("ntp");
    memset(databuf,0,200);
    sprintf(databuf,"'%s','%s','%s','%s',%d,%d,%d", ntp.getString("sNtpServers").c_str(),
                                                    ntp.getString("sTimeZone").c_str(),
                                                    ntp.getString("sTimeZoneFile").c_str(),
                                                    ntp.getString("sTimeZoneFileDst").c_str(),
                                                    ntp.getInt("iAutoMode"),
                                                    ntp.getInt("iAutoDst"),
                                                    ntp.getInt("iRefreshTime"));

    //printf("%s",databuf);
    g_free(mldb_insert(TABLE_NETWORK_NTP,(char*)"sNtpServers,sTimeZone,sTimeZoneFile,sTimeZoneFileDst,iAutoDst,iAutoMode,iRefreshTime",
                                           databuf));
    //4.设定 后台网址
    col_para =(char*) "id INTEGER PRIMARY KEY," \
               "device_reg_api TEXT NOT NULL," \
               "device_bind_api TEXT NOT NULL";
    g_free(mldb_create(TABLE_NETWORK_BKURL, col_para));
    YamlValue BkUrl =YamlConf->root().getSub("BackgroundUrl");
    memset(databuf,0,200);
    sprintf(databuf,"'%s','%s'", BkUrl.getString("device_reg_api").c_str(),
                                 BkUrl.getString("device_bind_api").c_str());

   // printf("%s",databuf);
    g_free(mldb_insert(TABLE_NETWORK_BKURL,(char*)"device_reg_api,device_bind_api",
                                           databuf));   

    //5.保存地图 
    col_para =(char*) "id INTEGER PRIMARY KEY," \
            "map_id INT DEFAULT 0," \
            "map_name TEXT NOT NULL,"\
            "map_info_path TEXT NOT NULL,"\
            "map_data_path TEXT NOT NULL";
    g_free(mldb_create(TABLE_NETWORK_MAPPATH, col_para));                                       
    // memset(databuf,0,200);
    // sprintf(databuf,"%d,'%s','%s','%s'",0,"房间"," "," ");
    // g_free(mldb_insert(TABLE_NETWORK_MAPPATH,(char*)"map_id,map_name,map_info_path,map_data_path",databuf));
    //6.设定清扫模式
    col_para =(char*) "id INTEGER PRIMARY KEY," \
               "mode INT DEFAULT 2";
    g_free(mldb_create(TABLE_NETWORK_CLEANTYPE, col_para));
    g_free(mldb_insert(TABLE_NETWORK_CLEANTYPE,(char*)"mode",(char*)"2"));  
    //7.设定清洁模式
    col_para =(char*) "id INTEGER PRIMARY KEY," \
               "mode INT DEFAULT 0,"\
               "suction INT DEFAULT 0,"\
               "water_yield INT DEFAULT 0,"\
               "speed INT DEFAULT 0,"\
               "area INT DEFAULT 0";
    g_free(mldb_create(TABLE_NETWORK_CLEANSTRENGTH, col_para));
    memset(databuf,0,200);
    sprintf(databuf,"%d,%d,%d,%d,%d",1,1,1,1,10);   
    g_free(mldb_insert(TABLE_NETWORK_CLEANSTRENGTH,(char*)"mode,suction,water_yield, \
                    speed,area",databuf));
    //7.设定勿扰模式
    col_para =(char*) "id INTEGER PRIMARY KEY," \
               "enable INT DEFAULT 0,"\
               "start_time INT DEFAULT 0,"\
               "end_time INT DEFAULT 0";
    g_free(mldb_create(TABLE_NETWORK_NODISTURB, col_para));
    memset(databuf,0,200);
    sprintf(databuf,"%d,%d,%d",0,0,0);   
    g_free(mldb_insert(TABLE_NETWORK_NODISTURB,(char*)"enable,start_time,end_time",databuf));    
    //8.设定预约清扫模式 ，用了2个table存储
    col_para =(char*) "id INTEGER PRIMARY KEY," \
               "mode INT DEFAULT 0,"\
               "enable INT DEFAULT 0";
    g_free(mldb_create(TABLE_NETWORK_TIMINGCLEANONOFF, col_para));
    memset(databuf,0,200);
    sprintf(databuf,"'%d','%d'",0,0);
    g_free(mldb_insert(TABLE_NETWORK_TIMINGCLEANONOFF,(char*)"mode,enable",databuf));
    col_para =(char*) "id INTEGER PRIMARY KEY," \
               "day INT DEFAULT 0,"\
               "enable INT DEFAULT 0,"\
               "start_time INT DEFAULT 0";
    g_free(mldb_create(TABLE_NETWORK_TIMINGCLEANCFG, col_para));
    memset(databuf,0,200);
    sprintf(databuf,"'%d','%d','%d'",1,0,0);
    g_free(mldb_insert(TABLE_NETWORK_TIMINGCLEANCFG,(char*)"day,enable,start_time",databuf)); 
    sprintf(databuf,"'%d','%d','%d'",2,0,0);
    g_free(mldb_insert(TABLE_NETWORK_TIMINGCLEANCFG,(char*)"day,enable,start_time",databuf)); 
    sprintf(databuf,"'%d','%d','%d'",3,0,0);
    g_free(mldb_insert(TABLE_NETWORK_TIMINGCLEANCFG,(char*)"day,enable,start_time",databuf)); 
    sprintf(databuf,"'%d','%d','%d'",4,0,0);
    g_free(mldb_insert(TABLE_NETWORK_TIMINGCLEANCFG,(char*)"day,enable,start_time",databuf)); 
    sprintf(databuf,"'%d','%d','%d'",5,0,0);
    g_free(mldb_insert(TABLE_NETWORK_TIMINGCLEANCFG,(char*)"day,enable,start_time",databuf)); 
    sprintf(databuf,"'%d','%d','%d'",6,0,0);
    g_free(mldb_insert(TABLE_NETWORK_TIMINGCLEANCFG,(char*)"day,enable,start_time",databuf)); 
    sprintf(databuf,"'%d','%d','%d'",0,0,0);
    g_free(mldb_insert(TABLE_NETWORK_TIMINGCLEANCFG,(char*)"day,enable,start_time",databuf)); 
    //9.更换自清洁模式
    col_para =(char*) "id INTEGER PRIMARY KEY," \
               "enable INT DEFAULT 0";
    g_free(mldb_create(TABLE_NETWORK_SELFCLEAN, col_para));
    memset(databuf,0,200);
    sprintf(databuf,"'%d'",0);
    g_free(mldb_insert(TABLE_NETWORK_SELFCLEAN,(char*)"enable",databuf));
    //10.更换区域配置
    col_para =(char*) "id INTEGER PRIMARY KEY," \
               "map_id INT DEFAULT 0,"\
               "room TEXT NOT NULL,"\
               "virtual_wall INT DEFAULT 0,"\
               "virtual_forbidden_zone TEXT NOT NULL,"\
               "virtual_threshold TEXT NOT NULL";
               
    g_free(mldb_create(TABLE_NETWORK_ZONECFG, col_para));
    //11.设置激活状态
    col_para =(char*) "id INTEGER PRIMARY KEY," \
               "activate INT DEFAULT 0";
    g_free(mldb_create(TABLE_NETWORK_ACTIVE, col_para));
    memset(databuf,0,200);
    sprintf(databuf,"%d",0);   
    g_free(mldb_insert(TABLE_NETWORK_ACTIVE,(char*)"activate",databuf));   
     
    
    free(databuf);
}

}
