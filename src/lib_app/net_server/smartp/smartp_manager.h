/**
 * @Description: app 通信
 * @Author:      zhz
 * @Version:     1
 */

#ifndef SRC_LIB_SMARTP_MANAGER_H_
#define SRC_LIB_SMARTP_MANAGER_H_

#include "smartp.h"
#include "smartplan.h"
#include "net_server/base/base_module.h"
#include "net_server/http/curl_manager.h"

namespace net {

#define APP_MSG_HDR(class_name, map_name, hdr_name) \
typedef int(class_name::*hdr_name) ( \
  Json::Value& req_json); \
typedef struct { \
  const int type; \
  hdr_name handler; \
} map_name;

class SmartpManager : public BaseModule,
                      public chml::MessageHandler,
                      public sigslot::has_slots<> {
 public:
  explicit SmartpManager(NetServer *net_server_ptr);
  virtual ~SmartpManager();

  int Start();
  int Stop();
  int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
 
 public:
  int UpdateDevicePose(Json::Value &jreq, Json::Value &jret);
  int UpdateTofBumpSensor(Json::Value &jreq, Json::Value &jret);
  int UpdateStatus(Json::Value &jreq, Json::Value &jret);
  int UpdateSelfCleanPercentage(Json::Value &jreq, Json::Value &jret);
  int SensorData(Json::Value &jreq, Json::Value &jret);
  int UpdateCleanArea(Json::Value &jreq, Json::Value &jret);
  int UpdateDeviceError(Json::Value &jreq, Json::Value &jret);
  int CleanLog(Json::Value &jreq, Json::Value &jret);
  
  int GetAllState(Json::Value& jreq);
  int GetTraj(Json::Value& jreq);
  int QueryMap(Json::Value& jreq);
  int ReadMap(Json::Value& jreq);
  int SelectMap(Json::Value& jreq);
  int ChangeMapName(Json::Value& jreq);
  int StartClean(Json::Value& jreq);
  int StartZoningClean(Json::Value& jreq);
  int PauseClean(Json::Value& jreq);
  int ContinueClean(Json::Value& jreq);
  int Recharge(Json::Value& jreq);
  int GetClearMode(Json::Value& jreq);
  int SetClearMode(Json::Value& jreq);
  int GetCleanMode(Json::Value& jreq);
  int SetCleanMode(Json::Value& jreq);
  int GetAudioCfg(Json::Value& jreq);
  int SetAudioCfg(Json::Value& jreq);
  int GetTimingCleanCfg(Json::Value& jreq);
  int SetTimingCleanCfg(Json::Value& jreq);
  int GetDonotDisturbCfg(Json::Value& jreq);
  int SetDonotDisturbCfg(Json::Value& jreq);
  int GetZoneCfg(Json::Value& jreq);
  int SetZoneCfg(Json::Value& jreq);
  int GetSelfCleanCfg(Json::Value& jreq);
  int SetSelfCleanCfg(Json::Value& jreq);
  int GetLog(Json::Value& jreq);
  int FastBuildMap(Json::Value& jreq);
  int DeleteMap(Json::Value& jreq);
  int ForceTime(Json::Value& jreq);
  int RestoreDefault(Json::Value& jreq);
  int ResetFactory(Json::Value& jreq);
  int OtaUpdate(Json::Value& jreq);

 private:
  void OnMessage(chml::Message* msg);
  void OnHttpResDone(CurlRequest::Ptr request);
  void OnHttpResErr(int code, CurlRequest::Ptr request);
  int StartDevServer();
  int UpdateBatteryLevel();
  int SendHttpData(std::string url, std::string type, 
                   Json::Value& json_data, int time_out = 60);
  static void OnDeviceAdd(device_t *device, const void* data, size_t length, void *arg);
  static void OnDeviceRemove(device_t *device, const void* data, size_t length, void *arg);
  static void OnCommandReceived(device_t * device, uint16 nid, const void* data, 
                                size_t length, void *arg);
  int GetMac();
  std::string GetGuid();
  bool GetMapData(Json::Value &json_data, std::vector<int8_t>& lz4_map, 
                  int& dest_len, int& map_size);
  int SendMap(Json::Value& jreq);
  int SendMsg(int id, Json::Value& json_body);
  int CommonSetCfg(std::string str_type, Json::Value& json_body);
  int GetStatus(Json::Value &jreq);
#ifdef UDP_TEST
  static void UdpTest(SmartpManager* arg);
#endif
#ifdef TCP_TEST
  static void TcpTest(SmartpManager* arg);
#endif
  int DealActivateRes(const std::string& str_res, bool success_res = true);
  int JudgeActivationStatus();
  int ActivateDev(); // 需要激活一次设备，直到激活成功

 private:
  chml::DpClient::Ptr dp_client_;
  smartp_t* smartp_;
  std::string mac_;
  std::string sn_;
  std::string aes_sn_;
  void* mqtt_;
  smartplan_t* lan_;
  chml::EventService::Ptr event_service_;
  chml::EventService::Ptr event_sdk_;
  CurlManager::Ptr curl_manager_;
  chml::CriticalSection c_mutex_;
  int battery_level_;
  bool battery_flag_;
};

}

#endif  // SRC_LIB_SMARTP_MANAGER_H_