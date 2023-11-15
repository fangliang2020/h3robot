#ifndef DB_NETWORK_H
#define DB_NETWORK_H

#include "db_server/base/base_module.h"
#include "db_server/parseyaml/yamlparse.h"

namespace db{

class Db_Network : public BaseModule {
public:
  explicit Db_Network(DbServer *db_server_ptr,YamlConfig *YamlConfig);
  virtual ~Db_Network();
  int Start();
  int Stop();
  int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
  void Network_DbInit(YamlConfig *yamlconfig);
  int Get_SN(Json::Value &jreq, Json::Value &jret);
  int Set_Clean_Type(Json::Value &jreq, Json::Value &jret);
  int Get_Clean_Type(Json::Value &jreq, Json::Value &jret);
  int Set_Clean_Strength(Json::Value &jreq, Json::Value &jret);
  int Get_Clean_Strength(Json::Value &jreq, Json::Value &jret);
  int Set_Donot_Disturb_Cfg(Json::Value &jreq, Json::Value &jret);
  int Get_Donot_Disturb_Cfg(Json::Value &jreq, Json::Value &jret);
  int Set_Timing_Clean_Cfg(Json::Value &jreq, Json::Value &jret);
  int Get_Timing_Clean_Cfg(Json::Value &jreq, Json::Value &jret);
  int Set_Self_Clean_Cfg(Json::Value &jreq, Json::Value &jret);
  int Get_Self_Clean_Cfg(Json::Value &jreq, Json::Value &jret);
  int Set_Zone_Cfg(Json::Value &jreq, Json::Value &jret);
  int Get_Zone_Cfg(Json::Value &jreq, Json::Value &jret);
  int Save_Map(Json::Value &jreq, Json::Value &jret);
  int Delete_Map(Json::Value &jreq, Json::Value &jret);
  int Query_Map(Json::Value &jreq, Json::Value &jret);
  int Edit_MapName(Json::Value &jreq, Json::Value &jret);
  int Set_Active(Json::Value &jreq, Json::Value &jret);
  int Get_Active(Json::Value &jreq, Json::Value &jret);
  int Set_Wifi(Json::Value &jreq, Json::Value &jret);
  int Get_Wifi(Json::Value &jreq, Json::Value &jret);
private:
  chml::DpClient::Ptr dp_client_;
  YamlConfig *YamlPtr_;
};

}


#endif
