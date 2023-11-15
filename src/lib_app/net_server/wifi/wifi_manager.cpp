/**
 * @Description: wifi
 * @Author:      zhz
 * @Version:     1
 */

#include <thread>
#include "wifi_manager.h"
#include "net_server/base/common.h"
#include "net_server/main/net_server.h"
#include "net_server/base/jkey_common.h"

namespace net {

chml::DpClient::Ptr WifiManager::dp_client_ = nullptr;

DP_MSG_HDR(WifiManager, DP_MSG_DEAL, pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[] = {
  { DP_TYPE_TRIGGER_DATA,          &WifiManager::TriggerData}
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

WifiManager::WifiManager(NetServer *net_server_ptr)
  : BaseModule(net_server_ptr) {
}

WifiManager::~WifiManager() {
}

int WifiManager::Start() {
  dp_client_ = netptr_->GetDpClientPtr();
  ASSERT_RETURN_FAILURE((nullptr == dp_client_), -1);
  set_wifi_info_callback((wifi_info_callback_t) WifiInfoCallback);
  StartWifi();

  return 0;
}

int WifiManager::Stop() {
  return 0;
}

int WifiManager::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
  int nret = -1;
  for (uint32 idx = 0; idx < g_dpmsg_table_size; ++idx) {
    if (stype == g_dpmsg_table[idx].type) {
      nret = (this->*g_dpmsg_table[idx].handler) (jreq[JKEY_BODY], jret);
      break;
    }
  }

  return nret;
}

int WifiManager::TriggerData(Json::Value &jreq, Json::Value &jret) {
  if (jreq[JKEY_BUTTONS].isInt()
      && jreq[JKEY_BUTTONS].asInt() == 4) {
    Json::Value json_res;
    std::string str_res;
    std::string str_req = R"req({"type":"get_sn"})req";
    dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
    if (String2Json(str_res, json_res)
        && json_res[JKEY_BODY][JKEY_SN].isString()) {
      PlayAudio("Q005");
      // TODO test
      // std::string sn = "SDJ2023055DATET000000001";
      std::string sn = json_res[JKEY_BODY][JKEY_SN].asString();
      system("killall wpa_supplicant udhcpc");
      ch_wifi_apstart((char *)sn.c_str());
      // ch_wifi_apstart((char *)json_res[JKEY_BODY][JKEY_SN].asCString());
    } else {
      DLOG_WARNING(MOD_EB, "get sn failed %s .", str_res.c_str());
    }
  }

  return 0;
}

int WifiManager::StartWifi() {
  Json::Value json_res;
  std::string str_res;
  std::string str_req = R"req({"type":"get_wifi"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_SSID].isString()
      && json_res[JKEY_BODY][JKEY_PASSWORD].isString()) {
    std::string ssid = json_res[JKEY_BODY][JKEY_SSID].asString();
    std::string password = json_res[JKEY_BODY][JKEY_PASSWORD].asString();
    wifi_connect((char *)ssid.c_str(), (char *)password.c_str());
  } else {
    DLOG_WARNING(MOD_EB, "get wifi failed %s .", str_res.c_str());
  }
  
  return 0;
}

void WifiManager::WifiInfoCallback(network_t *wifi_info) {
  ch_wifi_apstop();
  PlayAudio("Q079");
  Json::Value json_req;
  std::string str_req, str_res;
  json_req[JKEY_TYPE] = "set_wifi";
  json_req[JKEY_BODY][JKEY_SSID] = wifi_info->ssid;
  json_req[JKEY_BODY][JKEY_PASSWORD] = wifi_info->password;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  DLOG_INFO(MOD_EB, "ssid %s password %s", wifi_info->ssid, wifi_info->password);
  int ret = wifi_connect((char *)wifi_info->ssid, (char *)wifi_info->password);
  if (ret == 0) {
    ret = 1;
    PlayAudio("Q004");
    int activate = 0;
    Json::Value json_res;
    str_res = "";
    str_req = R"req({"type":"get_activate"})req";
    dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
    DLOG_INFO(MOD_EB, "activate %s", str_res.c_str());
    if (String2Json(str_res, json_res)
        && json_res[JKEY_BODY][JKEY_ACTIVATE].isInt()) {
      activate = json_res[JKEY_BODY][JKEY_ACTIVATE].asInt();
    }

    if (!activate) {
      json_res.clear();
      str_res = "";
      str_req = R"req({"type":"get_sn"})req";
      dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
      if (String2Json(str_res, json_res)
          && json_res[JKEY_BODY][JKEY_SN].isString()) {
        std::string sn = json_res[JKEY_BODY][JKEY_SN].asString();
        uint8_t HexKey[33] = {0};
        // TODO test
        // sn = "SDJ2023055DATET000000001";
#ifdef FORMAT_ENVIRONMENT
        int reg_ret = regist_device_new(HexKey, (char *)sn.c_str());
#else
        int reg_ret = regist_device_test(HexKey, (char *)sn.c_str());
#endif
        DLOG_INFO(MOD_EB, "reg_ret %d", reg_ret);
        if (reg_ret == 1000 || reg_ret == 3001) {
          str_req = R"req({"type":"set_activate","body":{"activate":1}})req";
          dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);
        }
      }
    }
  } else {
    ret = 0;
    PlayAudio("Q015");
  }

  json_req.clear();
  str_req = "";
  json_req[JKEY_TYPE] = "set_wifi_state";
  json_req[JKEY_BODY][JKEY_WIFI_STATE] = ret;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
}

int WifiManager::PlayAudio(std::string audio) {
  Json::Value json_req;
  std::string str_req, str_res;
  json_req[JKEY_TYPE] = "set_voice";
  json_req[JKEY_BODY][JKEY_VOICE_TTS] = audio;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  
  return 0;
}

}