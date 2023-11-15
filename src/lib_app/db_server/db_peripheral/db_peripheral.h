#ifndef DB_PERIPHERAL_H
#define DB_PERIPHERAL_H

#include "db_server/base/base_module.h"
#include "db_server/parseyaml/yamlparse.h"

namespace db{
  
class Db_Peripheral : public BaseModule {
public:
  explicit Db_Peripheral(DbServer *db_server_ptr,YamlConfig *YamlConf);
  virtual ~Db_Peripheral();
  int Start();
  int Stop();
  int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
  void Peripherals_DbInit(YamlConfig *yamlconfig);
  int Set_Imu_Offset(Json::Value &jreq, Json::Value &jret);
  int Get_Imu_Offset(Json::Value &jreq, Json::Value &jret);
  int Set_Odm_Offset(Json::Value &jreq, Json::Value &jret);
  int Get_Odm_Offset(Json::Value &jreq, Json::Value &jret);
  int Get_Wheel_Pid(Json::Value &jreq, Json::Value &jret);
  int Get_Fan_Pid(Json::Value &jreq, Json::Value &jret);
  int Get_Wheel_Param(Json::Value &jreq, Json::Value &jret);
  int Get_Machine_Param(Json::Value &jreq, Json::Value &jret);
  int Set_Audio_Cfg(Json::Value &jreq, Json::Value &jret);
  int Get_Audio_Cfg(Json::Value &jreq, Json::Value &jret);  
private:
  chml::DpClient::Ptr dp_client_;
  YamlConfig *YamlPtr_;
};


}


#endif
