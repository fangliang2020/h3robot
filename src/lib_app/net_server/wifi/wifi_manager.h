/**
 * @Description: wifi
 * @Author:      zhz
 * @Version:     1
 */

#ifndef SRC_LIB_WIFI_MANAGER_H_
#define SRC_LIB_WIFI_MANAGER_H_

#include "wifi_switch.h"
#include "net_server/base/base_module.h"
#include "eventservice/base/basictypes.h"

namespace net {

enum WifiState {
  WIFI_CONNECTED = 0,
  WIFI_CONFIGING,
  WIFI_NOT_CONNECTED
};

class WifiManager : public BaseModule {
 public:
  explicit WifiManager(NetServer *net_server_ptr);
  virtual ~WifiManager();

  int Start();
  int Stop();
  int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
 
 public:
  int TriggerData(Json::Value &jreq, Json::Value &jret);

 private:
  int StartWifi();
  static void WifiInfoCallback(network_t *wifi_info);
  static int PlayAudio(std::string audio);

 private:
  static chml::DpClient::Ptr dp_client_;
};

}

#endif  // SRC_LIB_WIFI_MANAGER_H_
