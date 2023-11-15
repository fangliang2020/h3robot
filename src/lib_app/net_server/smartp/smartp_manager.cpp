/**
 * @Description: app 通信
 * @Author:      zhz
 * @Version:     1
 */

#include <fstream>
#include <sstream>
#include <ext/stdio_filebuf.h>
#include "lz4.h"
#include "cmd_id.h"
#include "mqttprotocol.h"
#include "smartp_manager.h"
#include "eventservice/util/base64.h"
#include "eventservice/util/time_util.h"
#include "net_server/base/common.h"
#include "net_server/main/net_server.h"
#include "net_server/base/jkey_common.h"
#include "robot_schedule/schedule/DeviceStates.h"
#ifdef UDP_TEST
#include <thread>
#endif
#ifdef TCP_TEST
#include <fcntl.h>
#include <thread>
#endif

#define MAX_QUE_CONN_NM 5
#define MAX_SOCK_FD FD_SETSIZE

namespace net {

#define MSG_UPDATE_BATTERY_LEVEL 0x119
#define MSG_START_DEV_SERVER     0x120
#define MSG_ACTIVATE_DEV         0x911
#define MSG_JUDGE_ACTIVATE       0x400

#ifdef DEVELOP_ENVIRONMENT // 开发环境
#define MQTT_ADDRESS       "newmqtt.mymlsoft.com" // test server
#define HTTP_ACTIVATE_URL  "http://dev-envsplit.mymlsoft.com/cdc-whitegoods/device/active"
#define HTTP_CLEAN_LOG_URL "http://dev-envsplit.mymlsoft.com/gateway/sweepingRobot/device/cleanLog/saveCleanLogInfo"
#define HTTP_ERROR_LOG_URL "http://dev-envsplit.mymlsoft.com/gateway/sweepingRobot/device/errorLog/uploadErrorLog/" // 接sn
#endif

#ifdef FORMAT_ENVIRONMENT  // 正式环境
#define MQTT_ADDRESS       "mqtt.mymlsoft.com"  // formal server
#define HTTP_ACTIVATE_URL  "http://device-device.mymlsoft.com/cdc-whitegoods/device/active"
#define HTTP_CLEAN_LOG_URL "http://gateway.mymlsoft.com/gateway/sweepingRobot/device/cleanLog/saveCleanLogInfo"
#define HTTP_ERROR_LOG_URL "http://gateway.mymlsoft.com/gateway/sweepingRobot/device/errorLog/uploadErrorLog/"
#endif

#ifdef TEST_ENVIRONMENT    // 测试环境
#define MQTT_ADDRESS       "newmqtt.mymlsoft.com" // test server
#define HTTP_ACTIVATE_URL  "http://test-envsplit.mymlsoft.com/cdc-whitegoods/device/active"
#define HTTP_CLEAN_LOG_URL "http://test-envsplit.mymlsoft.com/gateway/sweepingRobot/device/cleanLog/saveCleanLogInfo"
#define HTTP_ERROR_LOG_URL "http://test-envsplit.mymlsoft.com/gateway/sweepingRobot/device/errorLog/uploadErrorLog/"
#endif

#define MQTT_PORT 1883
#define MQTT_PASSWORD "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJ0YW8xLmppYW5nIiwidXNlciI6InRvdSJ9._yv05gR0dAFJeDoEPI8Wo5qB01Gf-cM8_M1SbdoV9jQ"

DP_MSG_HDR(SmartpManager, DP_MSG_DEAL, pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[] = {
  { DP_TYPE_SENSOR_DATA,           &SmartpManager::SensorData},
  { DP_TYPE_ROBOT_DEVICE_POSE,     &SmartpManager::UpdateDevicePose},
  { DP_TYPE_ROBOT_DEVICE_STATE,    &SmartpManager::UpdateStatus},
  { DP_TYPE_SELF_CLEAN_PERCENTAGE, &SmartpManager::UpdateSelfCleanPercentage},
  { DP_TYPE_UPDATE_CLEAN_AREA,     &SmartpManager::UpdateCleanArea},
  { DP_TYPE_TOF_BUMP_SENSOR,       &SmartpManager::UpdateTofBumpSensor},
  { DP_FAULT_STATE,                &SmartpManager::UpdateDeviceError},
  { DP_TYPE_CLEAN_LOG,             &SmartpManager::CleanLog}
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

APP_MSG_HDR(SmartpManager, APP_MSG_DEAL, pFUNC_DealAppMsg)

static APP_MSG_DEAL g_appmsg_table[] = {
  { CMD_ID_GET_ALL_STATE,           &SmartpManager::GetAllState},
  { CMD_ID_FORCE_TIME,              &SmartpManager::ForceTime},
  { CMD_ID_READ_MAP,                &SmartpManager::ReadMap},
  { CMD_ID_UPDATE_CLEAN_TRAJECTORY, &SmartpManager::GetTraj},
  { CMD_ID_QUERY_MAP,               &SmartpManager::QueryMap},
  { CMD_ID_SELECT_MAP,              &SmartpManager::SelectMap},
  { CMD_ID_START_CLEAN,             &SmartpManager::StartClean},
  { CMD_ID_FAST_BUILD_MAP,          &SmartpManager::FastBuildMap},
  { CMD_ID_START_ZONING_CLEAN,      &SmartpManager::StartZoningClean},
  { CMD_ID_PAUSE_CLEAN,             &SmartpManager::PauseClean},
  { CMD_ID_CONTINUE_CLEAN,          &SmartpManager::ContinueClean},
  { CMD_ID_RECHARGE,                &SmartpManager::Recharge},
  { CMD_ID_SET_ZONE_CFG,            &SmartpManager::SetZoneCfg},
  { CMD_ID_CHANGE_MAP_NAME,         &SmartpManager::ChangeMapName},
  { CMD_ID_SET_CLEAR_MODE,          &SmartpManager::SetClearMode},
  { CMD_ID_SET_CLEAN_MODE,          &SmartpManager::SetCleanMode},
  { CMD_ID_SET_TIMING_CLEAN_CFG,    &SmartpManager::SetTimingCleanCfg},
  { CMD_ID_SET_DONOT_DISTURB_CFG,   &SmartpManager::SetDonotDisturbCfg},
  { CMD_ID_SET_AUDIO_CFG,           &SmartpManager::SetAudioCfg},
  { CMD_ID_SET_SELF_CLEAN_CFG,      &SmartpManager::SetSelfCleanCfg},
  { CMD_ID_GET_CLEAR_MODE,          &SmartpManager::GetClearMode},
  { CMD_ID_GET_CLEAN_MODE,          &SmartpManager::GetCleanMode},
  { CMD_ID_GET_AUDIO_CFG,           &SmartpManager::GetAudioCfg},
  { CMD_ID_GET_TIMING_CLEAN_CFG,    &SmartpManager::GetTimingCleanCfg},
  { CMD_ID_GET_DONOT_DISTURB_CFG,   &SmartpManager::GetDonotDisturbCfg},
  { CMD_ID_GET_ZONE_CFG,            &SmartpManager::GetZoneCfg},
  { CMD_ID_GET_SELF_CLEAN_CFG,      &SmartpManager::GetSelfCleanCfg},
  { CMD_ID_GET_LOG,                 &SmartpManager::GetLog},
  { CMD_ID_DELETE_MAP,              &SmartpManager::DeleteMap},
  { CMD_ID_RESTORE_DEFAULT,         &SmartpManager::RestoreDefault},
  { CMD_ID_RESET_FACTORY,           &SmartpManager::ResetFactory},
  { CMD_ID_OTA_UPDATE,              &SmartpManager::OtaUpdate}
};
static uint32 g_appmsg_table_size = ARRAY_SIZE(g_appmsg_table);

SmartpManager::SmartpManager(NetServer *net_server_ptr)
  : BaseModule(net_server_ptr) {
  dp_client_ = nullptr;
  smartp_ = nullptr;
  mqtt_ = nullptr;
  lan_ = nullptr;
  event_sdk_ = nullptr;
  curl_manager_ = nullptr;
  battery_level_ = -1;
  battery_flag_ = true;
}

SmartpManager::~SmartpManager() {
}

int SmartpManager::Start() {
  dp_client_ = netptr_->GetDpClientPtr();
  ASSERT_RETURN_FAILURE((nullptr == dp_client_), -1);
  event_service_ = netptr_->GetEventServicePtr();
  ASSERT_RETURN_FAILURE((nullptr == event_service_), -2);
  event_sdk_  = chml::EventService::CreateEventService(NULL, "event_sdk");
  ASSERT_RETURN_FAILURE((nullptr == event_sdk_), -3);
  chml::EventService::Ptr event_curl = chml::EventService::CreateEventService(NULL, "curl_push");
  ASSERT_RETURN_FAILURE(!event_curl, -4);
  curl_manager_.reset(new CurlManager(event_curl));
  ASSERT_RETURN_FAILURE(!curl_manager_, -5);
  curl_manager_->Start();
  curl_manager_->SignalDone.connect(this, &SmartpManager::OnHttpResDone);
  curl_manager_->SignalError.connect(this, &SmartpManager::OnHttpResErr);
  smartp_ = smartp_create((void *)this);
  ASSERT_RETURN_FAILURE((nullptr == smartp_), -6);
  event_sdk_->PostDelayed(2000, this, MSG_START_DEV_SERVER);
  event_service_->PostDelayed(5000, this, MSG_JUDGE_ACTIVATE);
#ifdef UDP_TEST // 协议测试
  std::thread udp_test_thread([this](){
    UdpTest(this);
	});
  udp_test_thread.detach();
#endif

#ifdef TCP_TEST
  std::thread tcp_test_thread([this](){
    TcpTest(this);
	});
  tcp_test_thread.detach();
#endif

  return 0;
}

int SmartpManager::Stop() {
  device_stop_server(smartp_);
  mqtt_destroy(mqtt_);
  smartp_lan_destroy(lan_);
  smartp_destroy(smartp_);
  
  return 0;
}

int SmartpManager::GetMac() {
  struct ifreq ifreq = {0};
  int sock = socket(AF_INET,SOCK_STREAM, 0);
  if (sock < 0){
    perror("socket ");
    return -1;
  }

  strncpy(ifreq.ifr_name, "wlan0", strlen("wlan0"));
  if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0) {
    perror( "ioctl ");
    close(sock);
    return -1;
  }
  close(sock);

  char mac[20] = {0};
  snprintf(mac, 20, "%02X%02X%02X%02X%02X%02X", ifreq.ifr_hwaddr.sa_data[0],
    ifreq.ifr_hwaddr.sa_data[1], ifreq.ifr_hwaddr.sa_data[2], ifreq.ifr_hwaddr.sa_data[3],
    ifreq.ifr_hwaddr.sa_data[4], ifreq.ifr_hwaddr.sa_data[5]);
  mac_ = mac;
  DLOG_INFO(MOD_EB, "mac : %s", mac_.c_str());

  return 0;
}

int SmartpManager::StartDevServer() {
  Json::Value json_res;
  std::string str_res;
  std::string str_req = R"req({"type":"get_sn"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_SN].isString()) {
    sn_ = json_res[JKEY_BODY][JKEY_SN].asString();
    // TODO test
    // sn_ = "SDJ2023055DATET000000001";
    mqtt_ = mqtt_init(MQTT_ADDRESS, MQTT_PORT, sn_.c_str(), MQTT_PASSWORD);
    ASSERT_RETURN_FAILURE((nullptr == mqtt_), -1);
    lan_ = smartp_lan_create();
    ASSERT_RETURN_FAILURE((nullptr == lan_), -1);
    smartp_protocol_register(smartp_, (protocol_t*)mqtt_);
    smartp_protocol_register(smartp_, (protocol_t*)lan_);
    smartp_set_callback(smartp_, (SmartPEvent )OnDeviceAdd, (SmartPEvent )OnDeviceRemove, 
      (SmartPCommandEvent )OnCommandReceived);
    device_start_server(smartp_, sn_.c_str());
    size_t out_length = 0;
    void *output = encrypt_data(sn_.c_str(), sn_.size(), &out_length); // 需要初始化之后这个接口才能正常调
    if (output != nullptr) {
      std::string out((char *)output, out_length);
      aes_sn_ = out;
      delete (char *)output;
    }
    DLOG_INFO(MOD_EB, "start dev server.");
  } else {
    DLOG_WARNING(MOD_EB, "get sn failed %s .", str_res.c_str());
    event_sdk_->PostDelayed(5000, this, MSG_START_DEV_SERVER);
  }

  return 0;
}

#ifdef UDP_TEST
void SmartpManager::UdpTest(SmartpManager* arg) {
  int sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  struct sockaddr_in server_addr, from_addr;
  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htons(INADDR_ANY);
  server_addr.sin_port = htons(9000);

  bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

  char buffer[4096] = {0};
  int ret = -1;
  socklen_t from_len = sizeof(struct sockaddr_in);
  while (true) {
    ret = recvfrom(sockfd, buffer, 4096, 0, (struct sockaddr*)&from_addr, &from_len);
    if (ret > 0) {
      OnCommandReceived(NULL, 0, (const void*) buffer, ret, arg);
    }
  }
}
#endif

#ifdef TCP_TEST
void SmartpManager::TcpTest(SmartpManager* arg)
{
  struct sockaddr_in server_sockaddr,client_sockaddr;
  unsigned int sin_size,count;
  fd_set inset,tmp_inset;//新增
  char buf[BUFFER_SIZE];
  int sockfd,client_fd,fd;
  sockfd=socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);
  int optval =1;
  setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(int));
  server_sockaddr.sin_family=AF_INET;
  server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_sockaddr.sin_port=htons(9000);
  bzero(&(server_sockaddr.sin_zero),8);
  while(bind(sockfd,(struct sockaddr*)&server_sockaddr,sizeof(server_sockaddr))==-1)
  {
    printf("Socketserver bind err\n");
    sleep(1);
  }
  printf("SocketServer bind success.\n");
  while (listen(sockfd,MAX_QUE_CONN_NM) == -1)
  {
      printf("SocketServer listen err\n");
      sleep(1);
  }
  printf("SocketServer listen success.\n"); 
  //4.调用socket()函数描述符作为文件描述符
  FD_ZERO(&inset);
  FD_SET(sockfd,&inset);
  while(true)
  {
    tmp_inset = inset; 
    sin_size=sizeof(struct sockaddr_in); 
    memset(buf, 0, sizeof(buf)); 
    /*调用 select()函数*/ 
    if(!(select(MAX_SOCK_FD, &tmp_inset, NULL, NULL, NULL) > 0)) 
    { 
      perror("select"); 
    } 
    for (fd = 0; fd < MAX_SOCK_FD; fd++) 
    { 
      if (FD_ISSET(fd, &tmp_inset) > 0) 
      { 
        if (fd == sockfd) 
        { /* 服务端接收客户端的连接请求 */ 
          if ((client_fd = accept(sockfd,(struct sockaddr *)&client_sockaddr, &sin_size))== -1) 
          { 
            perror("accept"); 
            exit(1); 
          } 
          FD_SET(client_fd, &inset); 
          printf("New connection from %d(socket)\n", client_fd); 
        } 
        else /* 处理从客户端发来的消息 */ 
        { 
          if ((count = recv(client_fd, buf, BUFFER_SIZE, 0)) > 0) 
          { 
            printf("Received a message from %d: %s\n", 
            client_fd, buf); 
          } 
          else 
          { 
            close(fd); 
            FD_CLR(fd, &inset); 
            printf("Client %d(socket) has left\n", fd);   

          } 
        } 
      } /* end of if FD_ISSET*/ 
    } /* end of for fd*/
  }
}

#endif

void SmartpManager::OnMessage(chml::Message* msg) {
  if (MSG_UPDATE_BATTERY_LEVEL == msg->message_id) {
    UpdateBatteryLevel();
    event_service_->PostDelayed(60000, this, MSG_UPDATE_BATTERY_LEVEL);
  } else if (MSG_START_DEV_SERVER == msg->message_id) {
    StartDevServer();
  } else if (MSG_ACTIVATE_DEV == msg->message_id) {
    ActivateDev();
  } else if (MSG_JUDGE_ACTIVATE == msg->message_id) {
    JudgeActivationStatus();
  }
}

int SmartpManager::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
  DLOG_DEBUG(MOD_EB, "OnDpMessage:%s.", stype.c_str());
  int nret = -1;
  for (uint32 idx = 0; idx < g_dpmsg_table_size; ++idx) {
    if (stype == g_dpmsg_table[idx].type) {
      nret = (this->*g_dpmsg_table[idx].handler) (jreq[JKEY_BODY], jret);
      break;
    }
  }

  return nret;
}

int SmartpManager::UpdateDevicePose(Json::Value &jreq, Json::Value &jret) {
  SendMsg(CMD_ID_UPDATE_DEVICE_POSE, jreq);
  
  return 0;
}

int SmartpManager::UpdateTofBumpSensor(Json::Value &jreq, Json::Value &jret) {
  SendMsg(CMD_ID_UPDATE_TOF_BUMP, jreq);
  
  return 0;
}

std::string SmartpManager::GetGuid() {
  static int num = 0;
  std::stringstream ss;
  ss << sn_ << chml::XTimeUtil::TimeUSecond() << (++num) % 10;

  return ss.str();
}

bool SmartpManager::GetMapData(Json::Value &json_data, std::vector<int8_t>& lz4_map, 
                               int& dest_len, int& map_size) {
  if (json_data[JKEY_MAP_INFO_PATH].isString()
      && json_data[JKEY_MAP_DATA_PATH].isString()) {
    std::string info_path = json_data[JKEY_MAP_INFO_PATH].asString();
    std::string data_dath = json_data[JKEY_MAP_DATA_PATH].asString();
    json_data.removeMember(JKEY_MAP_INFO_PATH);
    json_data.removeMember(JKEY_MAP_DATA_PATH);
    std::ifstream read_file(info_path);
    if (!read_file) {
      DLOG_ERROR(MOD_EB, "open %s fail !", info_path.c_str());
      return false;
    }
    int fd = static_cast< __gnu_cxx::stdio_filebuf<char > * const >(read_file.rdbuf())->fd();
    int ret = flock(fd, LOCK_SH);
    if (ret < 0) {
      DLOG_ERROR(MOD_EB, "flock %s fail !", info_path.c_str());
      return false;
    }
    float f_tmp = 0;
    int i_tmp = 0;
    read_file >> f_tmp;
    json_data[JKEY_RESOLUTION] = f_tmp;
    read_file >> i_tmp;
    json_data[JKEY_WIDTH] = i_tmp;
    read_file >> i_tmp;
    json_data[JKEY_HEIGHT] = i_tmp;
    read_file >> f_tmp;
    json_data[JKEY_X_MIN] = f_tmp;
    read_file >> f_tmp;
    json_data[JKEY_Y_MIN] = f_tmp;
    flock(fd, LOCK_UN);
    read_file.close();

    read_file.open(data_dath, std::ios::in);
    if (!read_file) {
      DLOG_ERROR(MOD_EB, "open %s fail !", data_dath.c_str());
      return false;
    }
    fd = static_cast< __gnu_cxx::stdio_filebuf<char > * const >(read_file.rdbuf())->fd();
    ret = flock(fd, LOCK_SH);
    if (ret < 0) {
      DLOG_ERROR(MOD_EB, "flock %s fail !", data_dath.c_str());
      return false;
    }
    flock(fd, LOCK_UN);
    int tmp;
    std::vector<int8_t> map_data;
    while (read_file >> tmp) {
      map_data.emplace_back((int8_t )tmp);
    }

    read_file.close();
    map_size = map_data.size();
    dest_len = LZ4_compressBound(map_size);
    lz4_map.resize(dest_len);
    dest_len = LZ4_compress_default((const char *)map_data.data(), 
                (char *)lz4_map.data(), map_size, dest_len);
    lz4_map.resize(dest_len);
    DLOG_INFO(MOD_EB, "dest_len : %d", dest_len);
  }
  
  return true;
}

int SmartpManager::SendMap(Json::Value &jreq) {
  std::vector<int8_t> lz4_map;
  int dest_len = 0, map_size = 0;
  if (GetMapData(jreq, lz4_map, dest_len, map_size)) {
    jreq[JKEY_GUID] = GetGuid(); // 分包发送地图时guid是相同的
    static int max_len = 60 * 1024 * 3 / 4; // 最大单包压缩数据长度，按base64后的长度60K计算
    int pack_num = dest_len / max_len;
    if (dest_len % max_len) {
      ++pack_num;
    }
    jreq[JKEY_PACK_NUM] = pack_num;
    int pos = 0, sub_num = 1;
    std::string dst;
    while (pack_num) {
      if (pack_num == 1) {
        jreq[JKEY_SUB_NUM] = sub_num;
        chml::Base64::EncodeFromArray(lz4_map.data() + pos, dest_len, &dst);
        jreq[JKEY_MAP] = dst;
        jreq[JKEY_MAP_ORIGINAL_SIZE] = map_size;
        DLOG_INFO(MOD_EB, "send map data size %d", dst.size());
        SendMsg(CMD_ID_UPLOAD_MAP, jreq);
      } else {
        jreq[JKEY_SUB_NUM] = sub_num++;
        chml::Base64::EncodeFromArray(lz4_map.data() + pos, max_len, &dst);
        jreq[JKEY_MAP] = dst;
        jreq[JKEY_MAP_ORIGINAL_SIZE] = map_size;
        DLOG_INFO(MOD_EB, "send map data size %d", dst.size());
        SendMsg(CMD_ID_UPLOAD_MAP, jreq);
        pos += max_len;
        dest_len -= max_len;
      }
      --pack_num;
    }
  }
  
  return 0;
}

int SmartpManager::GetStatus(Json::Value &jreq) {
  int JV_TO_INT(jreq, JKEY_DEVICE_STATE, dev_state, 1);
  if (ESTATE_CLEAN == dev_state) {
    int JV_TO_INT(jreq, JKEY_CLEAN_STATE, clean_state, 1);
    switch (clean_state) {
      case CLEAN_STATE_AUTO :
        return 2;
      case CLEAN_STATE_RECT :
        return 3;
      case CLEAN_STATE_LOCATION :
        return 4;
      case CLEAN_STATE_SITU :
        return 5;
      case CLEAN_STATE_RESERVATION :
        return 6;
      default :
        return 1;
    }
  } else {
    switch (dev_state) {
      case ESTATE_IDLE :
        return 1;
      case ESTATE_PAUSE_CLEAN :
        return 7;
      case ESTATE_BACK_CHARGE :
        return 8;
      case ESTATE_BACK_WASH :
        return 9;
      case ESTATE_OTA :
        return 10;
      case ESTATE_CHAGRE :
        return 11;
      case ESTATE_CHAGRE_END :
        return 12;
      case ESTATE_SHELF_CLEAN :
        return 13;
      case ESTATE_SHELF_CLEAN_END :
        return 14;
      case ESTATE_DORMANCY :
        return 15;
      case ESTATE_ERROR :
        return 16;
      case ESTATE_CHAGRE_PAUSE :
        return 17;
      case ESTATE_WASH_PAUSE :
        return 18;
      case ESTATE_DUMP :
        return 19;
      case ESTATE_DUMP_END :
        return 20;
      case ESTATE_SHUTDOWM :
        return 21;
      case ESTATE_FAST_BUILD_MAP :
        return 22;
      case ESTATE_PAUSE_FAST_BUILD_MAP :
        return 23;
      case ESTATE_FAST_BUILD_MAP_END :
        return 24;
      case ESTATE_BACK_CHARGE_ERROR :
        return 25;
      case ESTATE_BACK_WASH_ERROR :
        return 26;
      case ESTATE_LOCATE_ERROR :
        return 27;
      case ESTATE_DEVICE_TRAPPED :
        return 28;
      case ESTATE_FACTORY_TEST :
        return 29;
      default :
        return 1;
    }
  }
  
  return 1;
}

int SmartpManager::UpdateStatus(Json::Value &jreq, Json::Value &jret) {
  Json::Value json_data;
  json_data[JKEY_STATE] = GetStatus(jreq);
  SendMsg(CMD_ID_UPDATE_DEV_STATE, json_data);

  return -1;
}

int SmartpManager::UpdateSelfCleanPercentage(Json::Value &jreq, Json::Value &jret) {
  SendMsg(CMD_ID_UPDATE_SELF_CLEAN_PERCENTAGE, jreq);

  return 0;
}

int SmartpManager::SensorData(Json::Value &jreq, Json::Value &jret) {
  JV_TO_INT(jreq[JKEY_BATTERY], JKEY_ELECTRIC_QY, battery_level_, -1);
  if (battery_flag_ && battery_level_ > -1) {
    battery_flag_ = false;
    event_service_->Post(this, MSG_UPDATE_BATTERY_LEVEL);
  }

  return -1;
}

int SmartpManager::UpdateCleanArea(Json::Value &jreq, Json::Value &jret) {
  SendMsg(CMD_ID_UPDATE_AREA_AND_TIME, jreq);

  return 0;
}

int SmartpManager::UpdateDeviceError(Json::Value &jreq, Json::Value &jret) {
  SendMsg(CMD_ID_UPDATE_ALARM, jreq);
  
  return 0;
}

int SmartpManager::CleanLog(Json::Value &jreq, Json::Value &jret) {
  std::vector<int8_t> lz4_map;
  int dest_len = 0, map_size = 0;
  if (GetMapData(jreq, lz4_map, dest_len, map_size)) {
    jreq[JKEY_SN] = aes_sn_;
    std::string dst;
    chml::Base64::EncodeFromArray(lz4_map.data(), dest_len, &dst);
    jreq[JKEY_MAP] = dst;
    jreq[JKEY_MAP_ORIGINAL_SIZE] = map_size;
    SendHttpData(HTTP_CLEAN_LOG_URL, "clean_log", jreq);
  }
  
  return 0;
}

int SmartpManager::UpdateBatteryLevel() {
  if (battery_level_ >= 0) {
    Json::Value json_body;
    json_body[JKEY_BATTERY_LEVEL] = battery_level_;
    SendMsg(CMD_ID_UPDATE_BATTERY_LEVEL, json_body);
  }
  
  return 0;
}

void SmartpManager::OnDeviceAdd(device_t *device, const void* data, size_t length, void *arg) {

}

void SmartpManager::OnDeviceRemove(device_t *device, const void* data, size_t length, void *arg) {

}

void SmartpManager::OnCommandReceived(device_t * device, uint16 nid, const void* data, 
                                      size_t length, void *arg) {
  std::string str_req((const char *)data, length);
  DLOG_INFO(MOD_EB, "data:%s", str_req.c_str());
  Json::Value jreq;
  if (arg && String2Json(str_req, jreq)
      && jreq[JKEY_ID].isInt()) {
    int cmd_id = jreq[JKEY_ID].asInt();
    SmartpManager* live_this = (SmartpManager* ) arg;
    for (uint32 idx = 0; idx < g_appmsg_table_size; ++idx) {
      if (cmd_id == g_appmsg_table[idx].type) {
        (live_this->*g_appmsg_table[idx].handler) (jreq[JKEY_CONTENT]);
        return;
      }
    }
  } else {
    DLOG_WARNING(MOD_EB, "Data is not json or param err, data:%s.", str_req.c_str());
  }

  DLOG_WARNING(MOD_EB, "Don't find id, data:%s.", str_req.c_str());

  return;
}

int SmartpManager::SendMsg(int id, Json::Value& json_body) {
  Json::Value jreq;
  jreq[JKEY_ID] = id;
  jreq[JKEY_FROM] = "device";
  jreq[JKEY_FROM_NAME] = sn_;
  jreq[JKEY_TYPE] = "cmd";
  jreq[JKEY_CONTENT] = json_body;
  std::string str_msg;
  Json2String(jreq, str_msg);
  DLOG_INFO(MOD_EB, "send msg size %d data : %s.", str_msg.size(), str_msg.c_str());
  printf("send msg size %d data : %s.\n", (int)str_msg.size(), str_msg.c_str());
  chml::CritScope lock(&c_mutex_);
  device_send_to_all(smartp_, str_msg.c_str(), str_msg.size());
  
  return 0;
}

int SmartpManager::CommonSetCfg(std::string str_type, Json::Value& json_body) {
  std::string str_req;
  Json::Value json_req;
  json_req[JKEY_TYPE] = str_type;
  json_req[JKEY_BODY] = json_body;
  Json2String(json_req, str_req);

  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);
  
  return 0;
}

int SmartpManager::GetAllState(Json::Value& jreq) {
  std::string str_res;
  Json::Value json_req, json_res, json_data;
  std::string str_req = R"req({"type":"get_all_state"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)) {
    json_data[JKEY_STATE] = GetStatus(json_res[JKEY_BODY]);
    json_data[JKEY_CLEAN_AREA] = json_res[JKEY_BODY][JKEY_CLEAN_AREA];
    json_data[JKEY_CLEAN_TIME] = json_res[JKEY_BODY][JKEY_CLEAN_TIME];
    json_data[JKEY_SELF_CLEAN_PROGRESS] = json_res[JKEY_BODY][JKEY_SELF_CLEAN_PROGRESS];
    json_data[JKEY_CLEAN_ROBOT_POS] = json_res[JKEY_BODY][JKEY_CLEAN_ROBOT_POS];
    json_data[JKEY_CHARGE_BASE_POS] = json_res[JKEY_BODY][JKEY_CHARGE_BASE_POS];
    json_data[JKEY_CURRENT_MAP_ID] = json_res[JKEY_BODY][JKEY_CURRENT_MAP_ID];
  } else {
    DLOG_WARNING(MOD_EB, "get_all_state failed %s .", str_res.c_str());
    return 0;
  }

  if (battery_level_ >= 0) {
    json_data[JKEY_BATTERY_LEVEL] = battery_level_;
  } else {
    json_data[JKEY_BATTERY_LEVEL] = 80;
  }

  str_res = "";
  str_req = R"req({"type":"query_map"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  json_res.clear();
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_MAP].isArray()) {
    Json::Value json_tmp;
    int map_size = json_res[JKEY_BODY][JKEY_MAP].size();
    if (map_size == 0) {
      json_data[JKEY_MAP] = json_res[JKEY_BODY][JKEY_MAP];
    } else {
      json_req[JKEY_TYPE] = "get_zone_cfg";
      for (int i = 0; i < map_size; ++i) {
        json_req[JKEY_BODY][JKEY_MAP_ID] = json_res[JKEY_BODY][JKEY_MAP][i][JKEY_MAP_ID];
        str_req = "";
        str_res = "";
        Json2String(json_req, str_req);
        dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
        json_tmp.clear();
        if (String2Json(str_res, json_tmp)
            && json_tmp[JKEY_BODY][JKEY_MAP_ID].isInt()) {
          json_data[JKEY_MAP][i] = json_tmp[JKEY_BODY];
          json_data[JKEY_MAP][i][JKEY_NAME] = json_res[JKEY_BODY][JKEY_MAP][i][JKEY_MAP_NAME];
        }
      }
    }
  } else {
    DLOG_WARNING(MOD_EB, "query_map failed %s .", str_res.c_str());
    return 0;
  }
  
  str_res = "";
  str_req = R"req({"type":"get_clean_type"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  json_res.clear();
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_MODE].isInt()) {
    json_data[JKEY_CLEAN_MODE] = json_res[JKEY_BODY][JKEY_MODE];
  } else {
    DLOG_WARNING(MOD_EB, "get_clean_type failed %s .", str_res.c_str());
    return 0;
  }

  str_res = "";
  str_req = R"req({"type":"get_clean_strength"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  json_res.clear();
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_MODE].isInt()) {
    json_data[JKEY_CLEAN_STRENGTH] = json_res[JKEY_BODY];
  } else {
    DLOG_WARNING(MOD_EB, "get_clean_strength failed %s .", str_res.c_str());
    return 0;
  }

  str_res = "";
  str_req = R"req({"type":"get_audio_cfg"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  json_res.clear();
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_VOLUME].isInt()) {
    json_data[JKEY_AUDIO] = json_res[JKEY_BODY];
  } else {
    DLOG_WARNING(MOD_EB, "get_audio_cfg failed %s .", str_res.c_str());
    return 0;
  }

  str_res = "";
  str_req = R"req({"type":"get_donot_disturb_cfg"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  json_res.clear();
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_ENABLE].isInt()) {
    json_data[JKEY_DONOT_DISTURB] = json_res[JKEY_BODY];
  } else {
    DLOG_WARNING(MOD_EB, "get_donot_disturb_cfg failed %s .", str_res.c_str());
    return 0;
  }

  str_res = "";
  str_req = R"req({"type":"get_timing_clean_cfg"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  json_res.clear();
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_MODE].isInt()) {
    json_data[JKEY_TIMING_CLEAN] = json_res[JKEY_BODY];
  } else {
    DLOG_WARNING(MOD_EB, "get_timing_clean_cfg failed %s .", str_res.c_str());
    return 0;
  }

  str_res = "";
  str_req = R"req({"type":"get_self_clean_cfg"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  json_res.clear();
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_ENABLE].isInt()) {
    json_data[JKEY_SELF_CLEAN][JKEY_ENABLE] = json_res[JKEY_BODY][JKEY_ENABLE];
  } else {
    DLOG_WARNING(MOD_EB, "get_self_clean_cfg failed %s .", str_res.c_str());
    return 0;
  }

  SendMsg(CMD_ID_GET_ALL_STATE, json_data);

  return 0;
}

int SmartpManager::GetTraj(Json::Value& jreq) {
  std::string str_req, str_res;
  Json::Value json_req, json_res;
  json_req[JKEY_TYPE] = "update_clean_trail";
  json_req[JKEY_BODY] = jreq;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_START_POS].isInt()) {
    SendMsg(CMD_ID_UPDATE_CLEAN_TRAJECTORY, json_res[JKEY_BODY]);
  } else {
    DLOG_WARNING(MOD_EB, "update_clean_trail failed %s .", str_res.c_str());
  }
  
  return 0;
}

int SmartpManager::QueryMap(Json::Value& jreq) {
  std::string str_res;
  Json::Value json_res;
  std::string str_req = R"req({"type":"query_map"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_MAP].isArray()) {
    SendMsg(CMD_ID_QUERY_MAP, json_res[JKEY_BODY]);
  } else {
    DLOG_WARNING(MOD_EB, "query_map failed %s .", str_res.c_str());
  }
  
  return 0;
}

int SmartpManager::ReadMap(Json::Value& jreq) {
  std::string str_res;
  Json::Value json_res;
  if (jreq[JKEY_MAP_ID].isInt()) {
    std::string str_req = R"req({"type":"query_map"})req";
    dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
    if (String2Json(str_res, json_res)
        && json_res[JKEY_BODY][JKEY_MAP].isArray()) {
      int map_id = jreq[JKEY_MAP_ID].asInt();
      int size = json_res[JKEY_BODY][JKEY_MAP].size();
      for (int i = 0; i < size; ++i) {
        if (map_id == json_res[JKEY_BODY][JKEY_MAP][i][JKEY_MAP_ID].asInt()) {
          SendMap(json_res[JKEY_BODY][JKEY_MAP][i]);
          return 0;
        }
      }
    }
  }
  
  std::string str_req = R"req({"type":"read_current_map"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)) {
    SendMap(json_res[JKEY_BODY]);
  }
  
  return 0;
}

int SmartpManager::SelectMap(Json::Value& jreq) {
  std::string str_req;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "select_map";
  json_req[JKEY_BODY] = jreq;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);

  return 0;
}

int SmartpManager::ChangeMapName(Json::Value& jreq) {
  return CommonSetCfg("edit_map_name", jreq);
}

int SmartpManager::StartClean(Json::Value& jreq) {
  std::string str_req = R"req({"type":"start_clean"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);
  
  return 0;
}

int SmartpManager::StartZoningClean(Json::Value& jreq) {
  return CommonSetCfg("zoning_clean", jreq);
}

int SmartpManager::PauseClean(Json::Value& jreq) {
  std::string str_req = R"req({"type":"pause_clean"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);
  
  return 0;
}

int SmartpManager::ContinueClean(Json::Value& jreq) {
  std::string str_req = R"req({"type":"continue_clean"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);
  
  return 0;
}

int SmartpManager::Recharge(Json::Value& jreq) {
  std::string str_req = R"req({"type":"recharge"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);
  
  return 0;
}

int SmartpManager::GetClearMode(Json::Value& jreq) {
  std::string str_res;
  Json::Value json_res;
  std::string str_req = R"req({"type":"get_clean_type"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_MODE].isInt()) {
    SendMsg(CMD_ID_GET_CLEAR_MODE, json_res[JKEY_BODY]);
  } else {
    DLOG_WARNING(MOD_EB, "get_clean_type failed %s .", str_res.c_str());
  }
  
  return 0;
}

int SmartpManager::SetClearMode(Json::Value& jreq) {
  return CommonSetCfg("set_clean_type", jreq);
}

int SmartpManager::GetCleanMode(Json::Value& jreq) {
  std::string str_res;
  Json::Value json_res;
  std::string str_req = R"req({"type":"get_clean_strength"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_MODE].isInt()) {
    SendMsg(CMD_ID_GET_CLEAN_MODE, json_res[JKEY_BODY]);
  } else {
    DLOG_WARNING(MOD_EB, "get_clean_strength failed %s .", str_res.c_str());
  }
  
  return 0;
}

int SmartpManager::SetCleanMode(Json::Value& jreq) {
  return CommonSetCfg("set_clean_strength", jreq);
}

int SmartpManager::GetAudioCfg(Json::Value& jreq) {
  std::string str_res;
  Json::Value json_res;
  std::string str_req = R"req({"type":"get_audio_cfg"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_VOLUME].isInt()) {
    SendMsg(CMD_ID_GET_AUDIO_CFG, json_res[JKEY_BODY]);
  } else {
    DLOG_WARNING(MOD_EB, "get_audio_cfg failed % .", str_res.c_str());
  }
  
  return 0;
}

int SmartpManager::SetAudioCfg(Json::Value& jreq) {
  return CommonSetCfg("set_audio_cfg", jreq);
}

int SmartpManager::GetTimingCleanCfg(Json::Value& jreq) {
  std::string str_res;
  Json::Value json_res;
  std::string str_req = R"req({"type":"get_timing_clean_cfg"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_MODE].isInt()) {
    SendMsg(CMD_ID_GET_TIMING_CLEAN_CFG, json_res[JKEY_BODY]);
  } else {
    DLOG_WARNING(MOD_EB, "get_timing_clean_cfg failed %s .", str_res.c_str());
  }
  
  return 0;
}

int SmartpManager::SetTimingCleanCfg(Json::Value& jreq) {
  return CommonSetCfg("set_timing_clean_cfg", jreq);
}

int SmartpManager::GetDonotDisturbCfg(Json::Value& jreq) {
  std::string str_res;
  Json::Value json_res;
  std::string str_req = R"req({"type":"get_donot_disturb_cfg"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_ENABLE].isInt()) {
    SendMsg(CMD_ID_GET_DONOT_DISTURB_CFG, json_res[JKEY_BODY]);
  } else {
    DLOG_WARNING(MOD_EB, "get_donot_disturb_cfg failed %s .", str_res.c_str());
  }
  
  return 0;
}

int SmartpManager::SetDonotDisturbCfg(Json::Value& jreq) {
  return CommonSetCfg("set_donot_disturb_cfg", jreq);
}

int SmartpManager::GetZoneCfg(Json::Value& jreq) {
  std::string str_req, str_res;
  Json::Value json_req, json_res;
  json_req[JKEY_TYPE] = "get_zone_cfg";
  json_req[JKEY_BODY] = jreq;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_MAP_ID].isInt()) {
    SendMsg(CMD_ID_GET_ZONE_CFG, json_res[JKEY_BODY]);
  } else {
    DLOG_WARNING(MOD_EB, "get_zone_cfg failed %s .", str_res.c_str());
  }
  
  return 0;
}

int SmartpManager::SetZoneCfg(Json::Value& jreq) {
  return CommonSetCfg("set_zone_cfg", jreq);
}

int SmartpManager::GetSelfCleanCfg(Json::Value& jreq) {
  std::string str_res;
  Json::Value json_res;
  std::string str_req = R"req({"type":"get_self_clean_cfg"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_ENABLE].isInt()) {
    SendMsg(CMD_ID_GET_SELF_CLEAN_CFG, json_res[JKEY_BODY]);
  } else {
    DLOG_WARNING(MOD_EB, "get_self_clean_cfg failed %s .", str_res.c_str());
  }
  
  return 0;
}

int SmartpManager::SetSelfCleanCfg(Json::Value& jreq) { 
  return CommonSetCfg("set_self_clean_cfg", jreq);
}

int SmartpManager::GetLog(Json::Value& jreq) {
  uint32 sec = chml::XTimeUtil::TimeSecond();
  uint32 max_buffer_size = 5 * 1024 * 1024;
  LOG_TIME_S start = {sec - 30 * 24 * 3600, 0};
  LOG_TIME_S end = {sec, 0};
  LOG_QUERY_NODE qnode;
  qnode.is_first = true;
  qnode.qtype = LOG_QUERY_DESC_TYPE;
  chml::MemBuffer::Ptr res_buffer = chml::MemBuffer::CreateMemBuffer();
  Log_SearchBuffer(max_buffer_size, start, end, qnode, res_buffer);
  qnode.is_first = false;
  while (qnode.last_id) {
    qnode.start_id = qnode.last_id;
    Log_SearchBuffer(max_buffer_size, start, end, qnode, res_buffer);
  }

  std::string url = jreq[JKEY_URL].asString();
  DLOG_INFO(MOD_EB, "log url %s!", url.c_str());
  if (res_buffer->size() > 0) {
    auto request = curl_manager_->CreateRequest(HTTP_REQ_POST);
    bool ret = true;
    ret &= request->SetUrl(url);
    std::string name = "file";
    std::string file_name = sn_ + ".txt";
    std::string content_type = "text/plain";
    ret &= request->AddFile(name, res_buffer, file_name, content_type);
    ret &= request->SetConnectTimeout(20);
    ret &= request->SetTimeout(60);
    ret &= request->SetRetryTimes(3);
    request->SetUserType("dev_log");
    if (ret) {
      if (!curl_manager_->AddRequest(request)) {
        DLOG_ERROR(MOD_EB, "Post data %s failed!", url.c_str());
      }
    } else {
      DLOG_ERROR(MOD_EB, "%s pack failed !", url.c_str());
      LOG_POINT(161, 69, "%s pack failed !", url.c_str());
    }
  } else {
    DLOG_WARNING(MOD_EB, "search log is empty !");
  }

  return 0;
}

int SmartpManager::FastBuildMap(Json::Value& jreq) {
  std::string str_req = R"req({"type":"fast_build_map"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);
  
  return 0;
}

int SmartpManager::DeleteMap(Json::Value& jreq) {
  return CommonSetCfg("delete_map", jreq);
}

int SmartpManager::ForceTime(Json::Value& jreq) {
  // TODO 获取时区
  if (jreq[JKEY_TIME_ZONE].isInt()) {
    // TODO 对比时区
  }
  
  if (jreq[JKEY_FORCE_TIME].isString()) {
    struct timeval tv = {0};
    tv.tv_sec = chml::XTimeUtil::TimeLocalFromString(jreq[JKEY_FORCE_TIME].asString().c_str()) + 28800;
    settimeofday(&tv, NULL);
  }

  Json::Value json_data;
  json_data[JKEY_DEV_TIME] = chml::XTimeUtil::TimeLocalToString();
  SendMsg(CMD_ID_FORCE_TIME, json_data);

  return 0;
}

int SmartpManager::RestoreDefault(Json::Value& jreq) {
  std::string str_req = R"req({"type":"restore_default"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);
  
  return 0;
}

int SmartpManager::ResetFactory(Json::Value& jreq) {
  std::string str_req = R"req({"type":"reset_factory"})req";
  dp_client_->SendDpMessage(DP_DAEMON_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);
  
  return 0;
}

int SmartpManager::OtaUpdate(Json::Value& jreq) {
  // TODO 下载，md5比对
  std::string str_req = R"req({"type":"ota_update"})req";
  dp_client_->SendDpMessage(DP_DAEMON_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);
  
  return 0;
}

void SmartpManager::OnHttpResDone(CurlRequest::Ptr request) {
  DLOG_INFO(MOD_EB, "http success res %s", request->GetResponseData().c_str());
  if ("activate" == request->GetUserType()) {
    DealActivateRes(request->GetResponseData());
  }
}

void SmartpManager::OnHttpResErr(int code, CurlRequest::Ptr request) {
  DLOG_WARNING(MOD_EB, "http fail code %d res %s", code, request->GetResponseData().c_str());
  LOG_POINT(161, 69, "http fail code %d res %s", code, request->GetResponseData().c_str());
  if ("activate" == request->GetUserType()) {
    DealActivateRes(request->GetResponseData(), false);
  }
}

int SmartpManager::DealActivateRes(const std::string& str_res, bool success_res) {
  if (success_res) {
    event_service_->Clear(this, MSG_ACTIVATE_DEV);
    Json::Value json_res;
    if (String2Json(str_res, json_res)
        && json_res[JKEY_CODE].isString()
        && ("1000" == json_res[JKEY_CODE].asString()
        || "3001" == json_res[JKEY_CODE].asString())) {
      std::string str_req = R"req({"type":"set_activate","body":{"activate":1}})req";
      dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), NULL);
    }
  } else {
    event_service_->PostDelayed(10000, this, MSG_ACTIVATE_DEV);
  }

  return 0;
}

int SmartpManager::JudgeActivationStatus() {
  int activate = 0;
  Json::Value json_res;
  std::string str_res;
  std::string str_req = R"req({"type":"get_activate"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  DLOG_INFO(MOD_EB, "activate %s", str_res.c_str());
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_ACTIVATE].isInt()) {
    activate = json_res[JKEY_BODY][JKEY_ACTIVATE].asInt();
  }

  if (!activate) {
    ActivateDev();
  }

  return 0;
}

int SmartpManager::ActivateDev() {
  if (mac_.empty()) {
    GetMac();
  }

  Json::Value json_res, json_data;
  std::string str_res;
  std::string str_req = R"req({"type":"get_machine_param"})req";
  dp_client_->SendDpMessage(DP_DB_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_SW_VERSION].isString()
      && json_res[JKEY_BODY][JKEY_HW_VERSION].isString()) {
    json_data[JKEY_DEVICE][JKEY_HWVER] = json_res[JKEY_BODY][JKEY_HW_VERSION];
    json_data[JKEY_DEVICE][JKEY_SWVER] = json_res[JKEY_BODY][JKEY_SW_VERSION];
  }
  json_data[JKEY_DEVICE][JKEY_SN] = sn_;
  json_data[JKEY_DEVICE][JKEY_MAC] = mac_;
  SendHttpData(HTTP_ACTIVATE_URL, "activate", json_data, 10);
  
  return 0;
}

int SmartpManager::SendHttpData(std::string url, std::string type, 
                                Json::Value& json_data, int time_out) {
  std::string data;
  Json2String(json_data, data);
  DLOG_INFO(MOD_EB, "data %s", data.c_str());
  auto request = curl_manager_->CreateRequest(HTTP_REQ_POST);
  bool ret = true;
  ret &= request->SetUrl(url);
  ret &= request->AddHeader("Content-Type", "application/json");
  ret &= request->SetBodyData(data);
  ret &= request->SetConnectTimeout(20);
  ret &= request->SetTimeout(time_out);
  ret &= request->SetRetryTimes(3);
  request->SetUserType(type);
  if (ret) {
    if (!curl_manager_->AddRequest(request)) {
      DLOG_ERROR(MOD_EB, "Post data %s failed!", url.c_str());
    }
  } else {
    DLOG_ERROR(MOD_EB, "%s pack failed !", url.c_str());
    LOG_POINT(110, 110, "%s pack failed !", url.c_str());
  }

  return 0;
}

}