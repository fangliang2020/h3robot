#include "db_peripheral.h"
#include <glib.h>
#include "json-c/json.h"
#include "mldb.h"
#include "db_server/base/common.h"
#include "db_server/main/db_manager.h"

#define TABLE_PERIPHERALS_MACHINE         (char*)"machine"
#define TABLE_PERIPHERALS_WHEELPID        (char*)"WheelPid"
#define TABLE_PERIPHERALS_WHEELPARAM      (char*)"WheelParam"
#define TABLE_PERIPHERALS_FANPID          (char*)"FanPid"
#define TABLE_PERIPHERALS_MOPPID         (char*) "MopPid"
#define TABLE_PERIPHERALS_VERSION       (char*)"PeripheralsVersion"
#define TABLE_PERIPHERALS_IMUOFFSET     (char*)"imu_offset"
#define TABLE_PERIPHERALS_ODMOFFSET     (char*)"odm_offset"
#define TABLE_PERIPHERALS_AUDIO         (char*)"audio_cfg"
#define PERIPHERALS_VERSION             (char*)"1.0.0"

namespace db{
DP_MSG_HDR(Db_Peripheral,DP_MSG_DEAL,pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[]={
    {"set_imu_offset",   &Db_Peripheral::Set_Imu_Offset},
    {"get_imu_offset",   &Db_Peripheral::Get_Imu_Offset},
    {"set_odm_offset",   &Db_Peripheral::Set_Odm_Offset},
    {"get_odm_offset",   &Db_Peripheral::Get_Odm_Offset},
    {"get_wheel_pid",    &Db_Peripheral::Get_Wheel_Pid},
    {"get_fan_pid",      &Db_Peripheral::Get_Fan_Pid},
    {"get_wheel_param",  &Db_Peripheral::Get_Wheel_Param},
    {"get_machine_param",&Db_Peripheral::Get_Machine_Param},
    {"set_audio_cfg",    &Db_Peripheral::Set_Audio_Cfg},
    {"get_audio_cfg",    &Db_Peripheral::Get_Audio_Cfg}   
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

Db_Peripheral::Db_Peripheral(DbServer *db_server_ptr,YamlConfig *YamlConfig)
    : BaseModule(db_server_ptr),dp_client_(nullptr),YamlPtr_(YamlConfig) {
    dp_client_ = Dbptr_->GetDpClientPtr();
}
Db_Peripheral::~Db_Peripheral() {

}  

int Db_Peripheral::Start() {
  Peripherals_DbInit(YamlPtr_);
//   Json::Value jreq;
//   Json::Value jret;
//   Get_Fan_Pid(jreq,jret);
  return 0;
}

int Db_Peripheral::Stop() {
  return 0;
}

int Db_Peripheral::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
    DLOG_DEBUG(MOD_EB, "OnDpMessage:%s.", stype.c_str());
    int nret = -1;
    for (uint32 idx = 0; idx < g_dpmsg_table_size; ++idx) {
        if (stype == g_dpmsg_table[idx].type) {
        nret = (this->*g_dpmsg_table[idx].handler) (jreq["body"], jret["body"]);
        break;
        }
    }
    return nret;
}

int Db_Peripheral::Set_Imu_Offset(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq,"gyro_x_offset",gx_off,0);
    int JV_TO_INT(jreq,"gyro_y_offset",gy_off,0);
    int JV_TO_INT(jreq,"gyro_z_offset",gz_off,0);
    int JV_TO_INT(jreq,"accel_x_offset",ax_off,0);
    int JV_TO_INT(jreq,"accel_y_offset",ay_off,0);
    int JV_TO_INT(jreq,"accel_z_offset",az_off,0);   
    //写入db
    char *tempbuf=(char *)malloc(200);
    memset(tempbuf,0,200);
    sprintf(tempbuf,"gyro_x_offset=%d,gyro_y_offset=%d,gyro_z_offset=%d,\
                    accel_x_offset=%d,accel_y_offset=%d,accel_z_offset=%d\n",gx_off,gy_off,gz_off,ax_off,ay_off,az_off);
   // printf("%s",tempbuf); 
    g_free(mldb_update((char*)TABLE_PERIPHERALS_IMUOFFSET,tempbuf,(char*)"id=1"));
    free(tempbuf);
    return 0;
}

int Db_Peripheral::Get_Imu_Offset(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_PERIPHERALS_IMUOFFSET, NULL, (char*)"id=1", NULL, NULL);
    //printf("%s\n",json_str.c_str());
    if(String2Json(json_str,jtemp)) {
        jret["gyro_x_offset"] = std::stoi(jtemp["jData"][0]["gyro_x_offset"].asString());
        jret["gyro_y_offset"] = std::stoi(jtemp["jData"][0]["gyro_y_offset"].asString());
        jret["gyro_z_offset"] = std::stoi(jtemp["jData"][0]["gyro_z_offset"].asString());
        jret["accel_x_offset"] = std::stoi(jtemp["jData"][0]["accel_x_offset"].asString());
        jret["accel_y_offset"] = std::stoi(jtemp["jData"][0]["accel_y_offset"].asString());
        jret["accel_z_offset"] = std::stoi(jtemp["jData"][0]["accel_z_offset"].asString());
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0;
}

int Db_Peripheral::Set_Odm_Offset(Json::Value &jreq, Json::Value &jret) {
    double JV_TO_DOUBLE(jreq,"Wheel_L_offset",wl_off,0);
    double JV_TO_DOUBLE(jreq,"Wheel_R_offset",wr_off,0);
    double JV_TO_DOUBLE(jreq,"Wheel_DIS_offset",wd_off,0);
    //写入db
    char *tempbuf=(char *)malloc(200);
    memset(tempbuf,0,200);
    sprintf(tempbuf,"Wheel_L_offset=%3.2f,Wheel_R_offset=%3.2f,Wheel_DIS_offset=%3.2f",\
                    wl_off,wr_off,wd_off);
   // printf("%s",tempbuf); 
    g_free(mldb_update((char*)TABLE_PERIPHERALS_ODMOFFSET,tempbuf,(char*)"id=1"));
    free(tempbuf);
    return 0;
}

int Db_Peripheral::Get_Odm_Offset(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_PERIPHERALS_ODMOFFSET, NULL, (char*)"id=1", NULL, NULL);
    if(String2Json(json_str,jtemp)) {
        jret["Wheel_L_offset"] = std::stof(jtemp["jData"][0]["Wheel_L_offset"].asString());
        jret["Wheel_R_offset"] = std::stof(jtemp["jData"][0]["Wheel_R_offset"].asString());
        jret["Wheel_DIS_offset"] = std::stof(jtemp["jData"][0]["Wheel_DIS_offset"].asString());
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0;
}

int Db_Peripheral::Get_Wheel_Pid(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string id="id=";
    std::string id_temp;
    for(int i=0;i<4;i++) {
        id_temp=id+std::to_string(i+1);
        std::string json_str = mldb_select(TABLE_PERIPHERALS_WHEELPID, NULL,(char *)id_temp.c_str(), NULL, NULL);
        //  printf("jret : %s\n", json_str.c_str());
        if(String2Json(json_str,jtemp)) {
            jret["pid"][i]["DIF"] = std::stoi(jtemp["jData"][0]["DIF"].asString()); //0表示jData里面第一个数组
            jret["pid"][i]["KP"]  = std::stof(jtemp["jData"][0]["KP"].asString());
            jret["pid"][i]["KI"]  = std::stof(jtemp["jData"][0]["KI"].asString());
            jret["pid"][i]["KD"]  = std::stof(jtemp["jData"][0]["KD"].asString());
            //printf("jret : %s\n", jret.toStyledString().c_str());
        }
        else {
            return 2; 
        }
    }
//  printf("jret : %s\n", jret.toStyledString().c_str());
    return 0; 
}
int Db_Peripheral::Get_Fan_Pid(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_PERIPHERALS_FANPID, NULL, (char*)"id=1", NULL, NULL);
    if(String2Json(json_str,jtemp)) {
        jret["fKp"] = std::stof(jtemp["jData"][0]["fKp"].asString());
        jret["fKi"] = std::stof(jtemp["jData"][0]["fKi"].asString());
        jret["fKd"] = std::stof(jtemp["jData"][0]["fKd"].asString());
        jret["fPwmMax"] = std::stoi(jtemp["jData"][0]["fPwmMax"].asString());
        jret["fPwmMin"] = std::stoi(jtemp["jData"][0]["fPwmMin"].asString());
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0;
}
int Db_Peripheral::Get_Wheel_Param(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_PERIPHERALS_WHEELPARAM, NULL, (char*)"id=1", NULL, NULL);
    if(String2Json(json_str,jtemp)) {
        jret["wheel_code_disc"] = std::stof(jtemp["jData"][0]["WHEEL_CODE_DISC"].asString());
        jret["wheel_distance"] = std::stof(jtemp["jData"][0]["WHEEL_DISTANCE"].asString());
        jret["wheel_diameter"] = std::stof(jtemp["jData"][0]["WHEEL_DIAMETER"].asString());
        jret["length2encode_l"] = std::stoi(jtemp["jData"][0]["LENGTH2ENCODE_L"].asString());
        jret["length2encode_r"] = std::stoi(jtemp["jData"][0]["LENGTH2ENCODE_R"].asString());
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0;
}
int Db_Peripheral::Get_Machine_Param(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_PERIPHERALS_MACHINE, NULL, (char*)"id=1", NULL, NULL);
    if(String2Json(json_str,jtemp)) {
        jret["device_type"] = jtemp["jData"][0]["device_type"].asString();
        jret["device_sub_type"] = jtemp["jData"][0]["device_sub_type"].asString();
        jret["sw_version"] = jtemp["jData"][0]["sw_version"].asString();
        jret["hw_version"] = jtemp["jData"][0]["hw_version"].asString();
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0;   
}
int Db_Peripheral::Set_Audio_Cfg(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq,"volume",volume,0);
    int JV_TO_INT(jreq,"language",language,0);  
    //写入db
    char *tempbuf=(char *)malloc(50);
    memset(tempbuf,0,50);
    sprintf(tempbuf,"VOLUME=%d,LANGUAGE=%d\n",volume,language);
   // printf("%s",tempbuf); 
    g_free(mldb_update((char*)TABLE_PERIPHERALS_AUDIO,tempbuf,(char*)"id=1"));
    free(tempbuf);
    return 0;    
}
int Db_Peripheral::Get_Audio_Cfg(Json::Value &jreq, Json::Value &jret) {
    Json::Value jtemp;
    std::string json_str = mldb_select(TABLE_PERIPHERALS_AUDIO, NULL, (char*)"id=1", NULL, NULL);
    printf("%s\n",json_str.c_str());
    if(String2Json(json_str,jtemp)) {
        jret["volume"] = std::stoi(jtemp["jData"][0]["VOLUME"].asString());
        jret["language"] = std::stoi(jtemp["jData"][0]["LANGUAGE"].asString());
        DLOG_INFO(MOD_EB,"jret : %s\n", jret.toStyledString().c_str());
    }
    else {
        return 2; 
    }
    return 0;
}
void Db_Peripheral::Peripherals_DbInit(YamlConfig *YamlConf) {
    char *col_para;

    /*判断版本号，如果版本号没有更改则不需要写入数据库*/
    // if (equal_version((char*)TABLE_PERIPHERALS_VERSION, (char*)PERIPHERALS_VERSION))
    //     return;
    g_free(mldb_drop((char*)TABLE_PERIPHERALS_MACHINE));
    g_free(mldb_drop((char*)TABLE_PERIPHERALS_WHEELPID));
    g_free(mldb_drop((char*)TABLE_PERIPHERALS_WHEELPARAM));
    g_free(mldb_drop((char*)TABLE_PERIPHERALS_FANPID));
    g_free(mldb_drop((char*)TABLE_PERIPHERALS_MOPPID));
    g_free(mldb_drop((char*)TABLE_PERIPHERALS_VERSION));

    creat_version_table((char*)TABLE_PERIPHERALS_VERSION,(char*) PERIPHERALS_VERSION);
    
    col_para = (char*)"id INTEGER PRIMARY KEY AUTOINCREMENT," \
               "device_type TEXT DEFAULT ''," \
               "device_sub_type TEXT DEFAULT ''," \
               "sw_version TEXT DEFAULT ''," \
               "hw_version TEXT DEFAULT ''";  
    g_free(mldb_create((char*)TABLE_PERIPHERALS_MACHINE,col_para));
    char *tempbuf=(char *)malloc(200);
    YamlValue machine =YamlConf->root().getSub("machine");
    sprintf(tempbuf,"'%s','%s','%s','%s'",machine.getString("device_type").c_str(),
                                machine.getString("device_sub_type").c_str(),
                                machine.getString("sw_version").c_str(),
                                machine.getString("hw_version").c_str());
    g_free(mldb_insert((char*)TABLE_PERIPHERALS_MACHINE,(char*)"device_type,device_sub_type,sw_version,hw_version",tempbuf));
   
    //1.创建wheelpid
    col_para =(char*) "id INTEGER PRIMARY KEY," \
               "DIF INT DEFAULT 0," \
               "KP FLOAT DEFAULT 0," \
               "KI FLOAT DEFAULT 0," \
               "KD FLOAT DEFAULT 0 ";
            //    "LIMIT INT DEFAULT 0";
    g_free(mldb_create((char*)TABLE_PERIPHERALS_WHEELPID,col_para));
    memset(tempbuf,0,200);    
    const YAML::Node& wheelpid =YamlConf->m_root["wheelpid"];
    for(int i=0;i<4;i++){
        memset(tempbuf,0,200);
        const YAML::Node& nodetest=wheelpid[i];
        sprintf(tempbuf,"%d,%3.2f,%3.2f,%3.2f",nodetest["DIF"].as<int>(),
                                                nodetest["KP"].as<float>(),
                                                nodetest["KI"].as<float>(),
                                                nodetest["KD"].as<float>());
                                                // nodetest["LIMIT"].as<int>());
       // printf("%s",tempbuf);
    g_free(mldb_insert((char*)TABLE_PERIPHERALS_WHEELPID,(char*)"DIF,KP,KI,KD",tempbuf));
    }
    //2.创建wheelparam
    col_para =(char*) "id INTEGER PRIMARY KEY AUTOINCREMENT," \
               "WHEEL_CODE_DISC FLOAT DEFAULT 0," \
               "WHEEL_DISTANCE FLOAT DEFAULT 0," \
               "WHEEL_DIAMETER FLOAT DEFAULT 0," \
               "LENGTH2ENCODE_L INT DEFAULT 0," \
               "LENGTH2ENCODE_R INT DEFAULT 0"; 
    g_free(mldb_create((char*)TABLE_PERIPHERALS_WHEELPARAM,col_para));
    YamlValue wheelparam =YamlConf->root().getSub("wheelparam");
    memset(tempbuf,0,200);
    sprintf(tempbuf,"%3.1f,%2.4f,%2.4f,%d,%d", wheelparam.getFloat("WHEEL_CODE_DISC",30.0f),
                                               wheelparam.getFloat("WHEEL_DISTANCE",0.271f),
                                               wheelparam.getFloat("WHEEL_DIAMETER",0.0795f),
                                               wheelparam.getInt("LENGTH2ENCODE_L"),
                                               wheelparam.getInt("LENGTH2ENCODE_R"));
    //printf("%s",tempbuf);   
    g_free(mldb_insert((char*)TABLE_PERIPHERALS_WHEELPARAM,(char*)"WHEEL_CODE_DISC,WHEEL_DISTANCE,WHEEL_DIAMETER,LENGTH2ENCODE_L,LENGTH2ENCODE_R",(char*)tempbuf));
    //3.创建fanpid
    col_para =(char*) "id INTEGER PRIMARY KEY AUTOINCREMENT," \
               "fKp FLOAT DEFAULT ''," \
               "fKi FLOAT DEFAULT ''," \
               "fKd FLOAT DEFAULT ''," \
               "fPwmMax INT DEFAULT ''," \
               "fPwmMin INT DEFAULT ''";
    g_free(mldb_create((char*)TABLE_PERIPHERALS_FANPID, (char*)col_para));
    YamlValue fanpid =YamlConf->root().getSub("fanpid");
    memset(tempbuf,0,200);
    sprintf(tempbuf,"%3.2f,%3.2f,%3.2f,%d,%d",  fanpid.getFloat("fKp",100.0f),
                                                fanpid.getFloat("fKi",30.0f),
                                                fanpid.getFloat("fKd",20.0f),
                                                fanpid.getInt("fPwmMax"),
                                                fanpid.getInt("fPwmMin"));
   // printf("%s",tempbuf); 
    g_free(mldb_insert((char*)TABLE_PERIPHERALS_FANPID,(char*)"fKp,fKi,fKd,fPwmMax,fPwmMin",tempbuf));
    //3.创建 moppid
    col_para =(char*) "id INTEGER PRIMARY KEY AUTOINCREMENT," \
               "mKp FLOAT DEFAULT ''," \
               "mKi FLOAT DEFAULT ''," \
               "mKd FLOAT DEFAULT ''," \
               "mPwmMax INT DEFAULT ''," \
               "mPwmMin INT DEFAULT ''";
    g_free(mldb_create((char*)TABLE_PERIPHERALS_MOPPID,(char*)col_para));
    YamlValue moppid =YamlConf->root().getSub("moppid");
    memset(tempbuf,0,200);
    sprintf(tempbuf,"%3.2f,%3.2f,%3.2f,%d,%d",  moppid.getFloat("mKp",100.0f),
                                                moppid.getFloat("mKi",30.0f),
                                                moppid.getFloat("mKd",20.0f),
                                                moppid.getInt("mPwmMax"),
                                                moppid.getInt("mPwmMin"));
   // printf("%s",tempbuf); 
    g_free(mldb_insert((char*)TABLE_PERIPHERALS_MOPPID,(char*)"mKp,mKi,mKd,mPwmMax,mPwmMin",tempbuf)); 

    //4.imu offset
    col_para =(char*) "id INTEGER PRIMARY KEY," \
            "gyro_x_offset INT DEFAULT 0," \
            "gyro_y_offset INT DEFAULT 0,"\
            "gyro_z_offset INT DEFAULT 0,"\
            "accel_x_offset INT DEFAULT 0,"\
            "accel_y_offset INT DEFAULT 0,"\
            "accel_z_offset INT DEFAULT 0";
    g_free(mldb_create((char*)TABLE_PERIPHERALS_IMUOFFSET, col_para)); 
    memset(tempbuf,0,200);  
    sprintf(tempbuf,"%d,%d,%d,%d,%d,%d",0,0,0,0,0,1);
    g_free(mldb_insert((char*)TABLE_PERIPHERALS_IMUOFFSET,(char*)"gyro_x_offset,gyro_y_offset,gyro_z_offset, \
                    accel_x_offset,accel_y_offset,accel_z_offset",tempbuf));
    //5.odm offset
    col_para =(char*) "id INTEGER PRIMARY KEY," \
            "Wheel_L_offset FLOAT DEFAULT 0," \
            "Wheel_R_offset FLOAT DEFAULT 0," \
            "Wheel_DIS_offset FLOAT DEFAULT 0";
    g_free(mldb_create((char*)TABLE_PERIPHERALS_ODMOFFSET, col_para));
    memset(tempbuf,0,200);  
    sprintf(tempbuf,"%3.2f,%3.2f,%3.2f",0.2f,0.3f,0.4f);
    g_free(mldb_insert((char*)TABLE_PERIPHERALS_ODMOFFSET,(char*)"Wheel_L_offset,Wheel_R_offset,Wheel_DIS_offset",tempbuf));     
    //5.audio_cfg
    col_para =(char*) "id INTEGER PRIMARY KEY," \
            "VOLUME INT DEFAULT 0," \
            "LANGUAGE INT DEFAULT 0" ;
    g_free(mldb_create(TABLE_PERIPHERALS_AUDIO, col_para));
    memset(tempbuf,0,200);  
    sprintf(tempbuf,"%d,%d",50,1);
    g_free(mldb_insert(TABLE_PERIPHERALS_AUDIO,(char*)"VOLUME,LANGUAGE",tempbuf));   
    
    free(tempbuf);   
}

}
