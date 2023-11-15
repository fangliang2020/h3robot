/**
 * @Description: rpc pc调试
 * @Author:      zhz
 * @Version:     1
 */

#include <fstream>
#include <ext/stdio_filebuf.h>
#include "zlib.h"
#include "rpc_manager.h"
#include "net_server/base/common.h"
#include "net_server/main/net_server.h"
#include "net_server/base/jkey_common.h"
#include "eventservice/util/time_util.h"
#include "robot_schedule/schedule/DeviceStates.h"

namespace net {

rpc::client* RpcManager::c_reply_ = nullptr;
chml::CriticalSection RpcManager::c_mutex_;
chml::DpClient::Ptr RpcManager::dp_client_ = nullptr;
bool RpcManager::recv_connect_ = false;
int RpcManager::status_ = 0;

DP_MSG_HDR(RpcManager, DP_MSG_DEAL, pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[] = {
  { DP_TYPE_SENSOR_DATA,          &RpcManager::SendSensorData},
  { DP_TYPE_LASER_SENSOR_SCAN,    &RpcManager::SendPoseLaser},
  { DP_TYPE_ASTAR_PLAN,           &RpcManager::SendAstarPlan},
  { DP_TYPE_COVER_PLAN,           &RpcManager::SendCoverPlan},
  { DP_TYPE_ROBOT_DEVICE_STATE,   &RpcManager::SendNowState}
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

RpcManager::RpcManager(NetServer *net_server_ptr)
  : BaseModule(net_server_ptr) {
}

RpcManager::~RpcManager() {
  if (nullptr != c_reply_) {
    delete c_reply_;
    c_reply_ = nullptr;
  }
}

int RpcManager::Start() {
  dp_client_ = netptr_->GetDpClientPtr();
  ASSERT_RETURN_FAILURE((nullptr == dp_client_), -1);

  std::thread rpc_thread([](){
    RunRpcServer();
	});
  rpc_thread.detach();

  return 0;
}

int RpcManager::Stop() {
  return 0;
}

int RpcManager::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
  DLOG_DEBUG(MOD_EB, "OnDpMessage:%s.", stype.c_str());
  int nret = -1;
  if (recv_connect_) {
    for (uint32 idx = 0; idx < g_dpmsg_table_size; ++idx) {
      if (stype == g_dpmsg_table[idx].type) {
        nret = (this->*g_dpmsg_table[idx].handler) (jreq[JKEY_BODY], jret);
        break;
      }
    }
  }

  return nret;
}

void RpcManager::RunRpcServer() {
  rpc::server server(50001);

  server.bind("connect", Connect);
  server.bind("get_map", GetMap);
  server.bind("trajectory", GetTraj);
  server.bind("clear_traj", ClearTraj);

  server.bind("CommandGoForward", CmdGoForward);
  server.bind("CommandGoBackward", CmdGoBackward);
  server.bind("CommandTurnLeft", CmdTurnLeft);
  server.bind("CommandTurnRight", CmdTurnRight);
  server.bind("CommandMoveStop", CmdMoveStop);

  server.bind("CommandPause", CmdPause);
  server.bind("CommandResume", CmdResume);
  server.bind("CommandStop", CmdStop);
  server.bind("CommandGlobalClean", CmdGlobalClean);

  server.bind("CommandSetFanGear", CmdSetFanGear);
  server.bind("CommandSetMidGear", CmdSetMidGear);
  server.bind("CommandSetSideGear", CmdSetSideGear);
  server.bind("CommandMopUp", CmdMopUp);
  server.bind("CommandMopDown", CmdMopDown);
  server.bind("CommandToCharge", CmdToCharge);
  server.bind("CommandToWash", CmdToWash);
  server.bind("CommandLeftTofCalibrate", CmdLeftTofCalibrate);
  server.bind("CommandMidTofCalibrate", CmdMidTofCalibrate);
  server.bind("CommandRightTofCalibrate", CmdRightTofCalibrate);
  server.bind("CommandFrontTofDemarcate", CmdFrontTofDemarcate);
  server.bind("CommandBorderCalibrate", CmdBorderCalibrate);
  server.bind("CommandBorderDemarcate", CmdBorderDemarcate);
  server.bind("CommandImuCalibrate", CmdImuCalibrate);
  server.bind("CommandOdometerCalibrate", CmdOdometerCalibrate);
  server.bind("CommandGndCalibrate", CmdGndCalibrate);

  server.run();
}

bool RpcManager::Connect(int ip_a, int ip_b, int ip_c, int ip_d) {
  DLOG_INFO(MOD_BUS, "Connect ip : %d.%d.%d.%d", ip_a, ip_b, ip_c, ip_d);
  recv_connect_ = true;
  if (nullptr != c_reply_) {
    chml::CritScope lock(&c_mutex_);
    delete c_reply_;
    c_reply_ = nullptr;
  }

  std::string str_ip = std::to_string(ip_a) + '.' + std::to_string(ip_b) + '.' 
                       + std::to_string(ip_c) + '.' + std::to_string(ip_d);
  c_reply_ = new rpc::client(str_ip, 50001);
  
  return true;
}

RpcMap RpcManager::GetMap() {
  DLOG_INFO(MOD_BUS, "GetMap");
  RpcMap map;
  std::string str_res;
  Json::Value json_res;
  std::string str_req = R"req({"type":"read_current_map"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)
      && json_res[JKEY_BODY][JKEY_MAP_INFO_PATH].isString()
      && json_res[JKEY_BODY][JKEY_MAP_DATA_PATH].isString()) {
    std::string info_path = json_res[JKEY_BODY][JKEY_MAP_INFO_PATH].asString();
    std::string data_dath = json_res[JKEY_BODY][JKEY_MAP_DATA_PATH].asString();
    std::ifstream read_file(info_path);
    if (!read_file) {
      DLOG_ERROR(MOD_BUS, "open %s fail !", info_path.c_str());
      return map;
    }
    int fd = static_cast< __gnu_cxx::stdio_filebuf<char > * const >(read_file.rdbuf())->fd();
    int ret = flock(fd, LOCK_SH);
    if (ret < 0) {
      DLOG_ERROR(MOD_EB, "flock %s fail !", info_path.c_str());
      return map;
    }
    read_file >> map.resolution >> map.width >> map.height
      >> map.orig[0] >> map.orig[1] >> map.orig[2];
    flock(fd, LOCK_UN);
    read_file.close();

    read_file.open(data_dath, std::ios::in);
    if (!read_file) {
      DLOG_ERROR(MOD_BUS, "open %s fail !", data_dath.c_str());
      return map;
    }
    int tmp;
    std::vector<int8_t> map_data;
    fd = static_cast< __gnu_cxx::stdio_filebuf<char > * const >(read_file.rdbuf())->fd();
    ret = flock(fd, LOCK_SH);
    if (ret < 0) {
      DLOG_ERROR(MOD_EB, "flock %s fail !", info_path.c_str());
      return map;
    }
    while (read_file >> tmp) {
      map_data.emplace_back((int8_t )tmp);
    }
    flock(fd, LOCK_UN);
    read_file.close();
    
    uLong origin_map_size = map_data.size();
    uLong dest_len = compressBound(origin_map_size);
    map.map.resize(dest_len);
    compress((Bytef *)map.map.data(), &dest_len, (const Bytef *)map_data.data(), origin_map_size);
    map.map.resize(dest_len);
    DLOG_INFO(MOD_BUS, "send map data size %u", dest_len);
  }

  return map;
}

RpcAssetTraj RpcManager::GetTraj(int size) {
  DLOG_INFO(MOD_BUS, "GetTraj");
  RpcAssetTraj asset_traj;
  std::string str_res;
  Json::Value json_res;
  std::string str_req = R"req({"type":"get_robot_traj"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);
  if (String2Json(str_res, json_res)) {
    if (json_res[JKEY_BODY][JKEY_ROBOT_TRAJ].isArray()) {
      RpcTrajPose pose;
      int traj_size = json_res[JKEY_BODY][JKEY_ROBOT_TRAJ].size();
      for (int i = 0; i < traj_size; ++i) {
        pose.x = json_res[JKEY_BODY][JKEY_ROBOT_TRAJ][i][0].asFloat();
        pose.y = json_res[JKEY_BODY][JKEY_ROBOT_TRAJ][i][1].asFloat();
        pose.theta = json_res[JKEY_BODY][JKEY_ROBOT_TRAJ][i][2].asFloat();
        asset_traj.traj.emplace_back(pose);
      }
    }
  }

  return asset_traj;
}

void RpcManager::ClearTraj() {
  DLOG_INFO(MOD_BUS, "ClearTraj");
  std::string str_res;
  std::string str_req = R"req({"type":"clear_traj"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdGoForward() {
  DLOG_INFO(MOD_BUS, "CmdGoForward");
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "set_v_w";
  json_req[JKEY_BODY][JKEY_BRAKE] = 0;
  json_req[JKEY_BODY][JKEY_V] = 180;
  json_req[JKEY_BODY][JKEY_W] = 0;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdGoBackward() {
  DLOG_INFO(MOD_BUS, "CmdGoBackward");
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "set_v_w";
  json_req[JKEY_BODY][JKEY_BRAKE] = 0;
  json_req[JKEY_BODY][JKEY_V] = -90;
  json_req[JKEY_BODY][JKEY_W] = 0;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdTurnLeft() {
  DLOG_INFO(MOD_BUS, "CmdTurnLeft");
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "set_v_w";
  json_req[JKEY_BODY][JKEY_BRAKE] = 0;
  json_req[JKEY_BODY][JKEY_V] = 0;
  json_req[JKEY_BODY][JKEY_W] = -100;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdTurnRight() {
  DLOG_INFO(MOD_BUS, "CmdTurnRight");
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "set_v_w";
  json_req[JKEY_BODY][JKEY_BRAKE] = 0;
  json_req[JKEY_BODY][JKEY_V] = 0;
  json_req[JKEY_BODY][JKEY_W] = 100;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdMoveStop() {
  DLOG_INFO(MOD_BUS, "CmdMoveStop");
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "set_v_w";
  json_req[JKEY_BODY][JKEY_BRAKE] = 0;
  json_req[JKEY_BODY][JKEY_V] = 0;
  json_req[JKEY_BODY][JKEY_W] = 0;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdPause() {
  DLOG_INFO(MOD_BUS, "CmdPause");
  std::string str_res;
  std::string str_req = R"req({"type":"pause_clean"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdResume() {
  DLOG_INFO(MOD_BUS, "CmdResume");
  std::string str_res;
  std::string str_req = R"req({"type":"continue_clean"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdStop() {
  DLOG_INFO(MOD_BUS, "CmdStop");
  std::string str_res;
  std::string str_req = R"req({"type":"stop_clean"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdGlobalClean() {
  DLOG_INFO(MOD_BUS, "CmdGlobalClean");
  std::string str_res;
  std::string str_req = R"req({"type":"start_clean"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdSetFanGear(int gear) {
  DLOG_INFO(MOD_BUS, "CmdSetFanGear gear %d", gear);
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "set_fan_level";
  json_req[JKEY_BODY][JKEY_LEVEL] = gear;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdSetMidGear(int gear) {
  DLOG_INFO(MOD_BUS, "CmdSetMidGear gear %d", gear);
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "set_mid_level";
  json_req[JKEY_BODY][JKEY_LEVEL] = gear;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdSetSideGear(int gear) {
  DLOG_INFO(MOD_BUS, "CmdSetSideGear gear %d", gear);
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "set_side_level";
  json_req[JKEY_BODY][JKEY_LEVEL] = gear;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdMopUp() {
  DLOG_INFO(MOD_BUS, "CmdMopUp");
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "set_mop_ctrl";
  json_req[JKEY_BODY][JKEY_CTRL] = 0;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdMopDown() {
  DLOG_INFO(MOD_BUS, "CmdMopDown");
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "set_mop_ctrl";
  json_req[JKEY_BODY][JKEY_CTRL] = 1;
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdToCharge() {
  DLOG_INFO(MOD_BUS, "CmdToCharge");
  std::string str_res;
  std::string str_req = R"req({"type":"recharge"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdToWash() {
  DLOG_INFO(MOD_BUS, "CmdToWash");
  std::string str_res;
  std::string str_req = R"req({"type":"to_wash"})req";
  dp_client_->SendDpMessage(DP_SCHEDULE_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdLeftTofCalibrate() {
  DLOG_INFO(MOD_BUS, "CmdLeftTofCalibrate");
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "tof_calibrate";
  json_req[JKEY_BODY][JKEY_ORIENT] = "left";
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdMidTofCalibrate() {
  DLOG_INFO(MOD_BUS, "CmdMidTofCalibrate");
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "tof_calibrate";
  json_req[JKEY_BODY][JKEY_ORIENT] = "mid";
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdRightTofCalibrate() {
  DLOG_INFO(MOD_BUS, "CmdRightTofCalibrate");
  std::string str_req, str_res;
  Json::Value json_req;
  json_req[JKEY_TYPE] = "tof_calibrate";
  json_req[JKEY_BODY][JKEY_ORIENT] = "right";
  Json2String(json_req, str_req);
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdFrontTofDemarcate() {
  DLOG_INFO(MOD_BUS, "CmdFrontTofDemarcate");
  std::string str_res;
  std::string str_req = R"req({"type":"tof_demarcate"})req";
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdBorderCalibrate() {
  DLOG_INFO(MOD_BUS, "CmdBorderCalibrate");
  std::string str_res;
  std::string str_req = R"req({"type":"board_calibrate"})req";
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdBorderDemarcate() {
  DLOG_INFO(MOD_BUS, "CmdBorderDemarcate");
  std::string str_res;
  std::string str_req = R"req({"type":"board_demarcate"})req";
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdImuCalibrate() {
  DLOG_INFO(MOD_BUS, "CmdImuCalibrate");
  std::string str_res;
  std::string str_req = R"req({"type":"imu_calibrate"})req";
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdOdometerCalibrate() {
  DLOG_INFO(MOD_BUS, "CmdBorderDemarcate");
  std::string str_res;
  std::string str_req = R"req({"type":"odm_calibrate"})req";
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

void RpcManager::CmdGndCalibrate() {
  DLOG_INFO(MOD_BUS, "CmdGndCalibrate");
  std::string str_res;
  std::string str_req = R"req({"type":"gnddet_calibrate"})req";
  dp_client_->SendDpMessage(DP_PERIPHERALS_REQUEST, 0, str_req.c_str(), str_req.size(), &str_res);

  return;
}

int RpcManager::SendNowState(Json::Value &jreq, Json::Value &jret) {
  if (c_reply_ == nullptr || c_reply_->get_connection_state() != rpc::client::connection_state::connected) {
    DLOG_WARNING(MOD_BUS, "---------rpc connection lost. SendNowState----------");
    return 0;
  }
  RpcState nowState;
  int JV_TO_INT(jreq, JKEY_DEVICE_STATE, dev_state, 1);
  status_ = dev_state;
  if (ESTATE_CLEAN == dev_state) {
    int JV_TO_INT(jreq, JKEY_CLEAN_STATE, clean_state, 1);
    switch (clean_state) {
      case CLEAN_STATE_AUTO :
        nowState.state = "CLEAN_STATE_AUTO";
        break;
      case CLEAN_STATE_RECT :
        nowState.state = "CLEAN_STATE_RECT";
        break;
      case CLEAN_STATE_LOCATION :
        nowState.state = "CLEAN_STATE_LOCATION";
        break;
      case CLEAN_STATE_SITU :
        nowState.state = "CLEAN_STATE_SITU";
        break;
      case CLEAN_STATE_RESERVATION :
        nowState.state = "CLEAN_STATE_RESERVATION";
        break;
      case CLEAN_STATE_NULL :
        nowState.state = "CLEAN_STATE_NULL";
        break;
      default :
        return 0;
    }
  } else {
    switch (dev_state) {
      case ESTATE_IDLE :
        nowState.state = "ESTATE_IDLE";
        break;
      case ESTATE_PAUSE_CLEAN :
        nowState.state = "ESTATE_PAUSE_CLEAN";
        break;
      case ESTATE_BACK_CHARGE :
        nowState.state = "ESTATE_BACK_CHARGE";
        break;
      case ESTATE_BACK_WASH :
        nowState.state = "ESTATE_BACK_WASH";
        break;
      case ESTATE_OTA :
        nowState.state = "ESTATE_OTA";
        break;
      case ESTATE_CHAGRE :
        nowState.state = "ESTATE_CHAGRE";
        break;
      case ESTATE_CHAGRE_END :
        nowState.state = "ESTATE_CHAGRE_END";
        break;
      case ESTATE_SHELF_CLEAN :
        nowState.state = "ESTATE_SHELF_CLEAN";
        break;
      case ESTATE_SHELF_CLEAN_END :
        nowState.state = "ESTATE_SHELF_CLEAN_END";
        break;
      case ESTATE_DORMANCY :
        nowState.state = "ESTATE_DORMANCY";
        break;
      case ESTATE_ERROR :
        nowState.state = "ESTATE_ERROR";
        break;
      case ESTATE_CHAGRE_PAUSE :
        nowState.state = "ESTATE_CHAGRE_PAUSE";
        break;
      case ESTATE_WASH_PAUSE :
        nowState.state = "ESTATE_WASH_PAUSE";
        break;
      case ESTATE_DUMP :
        nowState.state = "ESTATE_DUMP";
        break;
      case ESTATE_DUMP_END :
        nowState.state = "ESTATE_DUMP_END";
        break;
      case ESTATE_SHUTDOWM :
        nowState.state = "ESTATE_SHUTDOWM";
        break;
      case ESTATE_FAST_BUILD_MAP :
        nowState.state = "ESTATE_FAST_BUILD_MAP";
        break;
      case ESTATE_PAUSE_FAST_BUILD_MAP :
        nowState.state = "ESTATE_PAUSE_FAST_BUILD_MAP";
        break;
      case ESTATE_FAST_BUILD_MAP_END :
        nowState.state = "ESTATE_FAST_BUILD_MAP_END";
        break;
      case ESTATE_BACK_CHARGE_ERROR :
        nowState.state = "ESTATE_BACK_CHARGE_ERROR";
        break;
      case ESTATE_BACK_WASH_ERROR :
        nowState.state = "ESTATE_BACK_WASH_ERROR";
        break;
      case ESTATE_LOCATE_ERROR :
        nowState.state = "ESTATE_LOCATE_ERROR";
        break;
      case ESTATE_DEVICE_TRAPPED :
        nowState.state = "ESTATE_DEVICE_TRAPPED";
        break;
      default :
        return 0;
    }
  }
  c_reply_->async_call("send_now_state", nowState);

  return 0;
}

int RpcManager::SendPoseLaser(Json::Value &jreq, Json::Value &jret) {
  if (c_reply_ == nullptr || c_reply_->get_connection_state() != rpc::client::connection_state::connected) {
    DLOG_WARNING(MOD_BUS, "---------rpc connection lost. SendPoseLaser----------");
    return 0;
  }

  RpcPoseLaser pose_laser;
  if (jreq[JKEY_LASER_POSE].isArray()) {
    RpcPose pose;
    int range_size = jreq[JKEY_LASER_POSE].size();
    for (int i = 0; i < range_size; ++i) {
      pose.x = jreq[JKEY_LASER_POSE][i][0].asFloat();
      pose.y = jreq[JKEY_LASER_POSE][i][1].asFloat();
      pose_laser.laser.emplace_back(pose);
    }
  }

  c_reply_->async_call("send_pose_laser", pose_laser);

  return 0;
}

int RpcManager::SendSensorData(Json::Value &jreq, Json::Value &jret) {
  if (c_reply_ == nullptr || c_reply_->get_connection_state() != rpc::client::connection_state::connected) {
    DLOG_WARNING(MOD_BUS, "---------rpc connection lost. SendSensorData----------");
    return 0;
  }

  RpcSensorData sensor_data;
  chml::TimeS tl = {0};
  chml::XTimeUtil::TimeLocalMake(&tl);
  sensor_data.tv_sec = tl.sec;
  sensor_data.tv_nsec = tl.usec * 1000;
  sensor_data.status = status_;
  JV_TO_INT(jreq[JKEY_VL6180], JKEY_DIS, sensor_data.dis, 0);
  JV_TO_DOUBLE(jreq[JKEY_IMU], JKEY_X_ACCEL, sensor_data.x_accel, 0);
  JV_TO_DOUBLE(jreq[JKEY_IMU], JKEY_Y_ACCEL, sensor_data.y_accel, 0);
  JV_TO_DOUBLE(jreq[JKEY_IMU], JKEY_Z_ACCEL, sensor_data.z_accel, 0);
  JV_TO_DOUBLE(jreq[JKEY_IMU], JKEY_X_GYRO, sensor_data.x_gyro, 0);
  JV_TO_DOUBLE(jreq[JKEY_IMU], JKEY_Y_GYRO, sensor_data.y_gyro, 0);
  JV_TO_DOUBLE(jreq[JKEY_IMU], JKEY_Z_GYRO, sensor_data.z_gyro, 0);
  JV_TO_DOUBLE(jreq[JKEY_IMU], JKEY_COURSE_ANGLE, sensor_data.course_angle, 0);
  JV_TO_INT(jreq[JKEY_BATTERY], JKEY_ELECTRIC_QY, sensor_data.battery, 0);
  JV_TO_DOUBLE(jreq[JKEY_GND_DET], JKEY_GNDDET_LF, sensor_data.lf_gnd, 0);
  JV_TO_DOUBLE(jreq[JKEY_GND_DET], JKEY_GNDDET_LB, sensor_data.lb_gnd, 0);
  JV_TO_DOUBLE(jreq[JKEY_GND_DET], JKEY_GNDDET_RF, sensor_data.rf_gnd, 0);
  JV_TO_DOUBLE(jreq[JKEY_GND_DET], JKEY_GNDDET_RB, sensor_data.rb_gnd, 0);
  JV_TO_DOUBLE(jreq[JKEY_CURRENT], JKEY_WHEEL_L_CURR, sensor_data.left_wheel, 0);
  JV_TO_DOUBLE(jreq[JKEY_CURRENT], JKEY_WHEEL_R_CURR, sensor_data.right_wheel, 0);
  JV_TO_DOUBLE(jreq[JKEY_CURRENT], JKEY_FAN_CURR, sensor_data.fan_motor, 0);
  JV_TO_DOUBLE(jreq[JKEY_CURRENT], JKEY_MID_CURR, sensor_data.mid_motor, 0);
  JV_TO_DOUBLE(jreq[JKEY_CURRENT], JKEY_SIDE_L_CURR, sensor_data.left_side, 0);
  JV_TO_DOUBLE(jreq[JKEY_CURRENT], JKEY_SIDE_R_CURR, sensor_data.right_side, 0);
  JV_TO_DOUBLE(jreq[JKEY_CURRENT], JKEY_MOP_CURR, sensor_data.mop_motor, 0);
  JV_TO_INT(jreq[JKEY_ODM], JKEY_ODM_LEFT, sensor_data.left_code, 0);
  JV_TO_INT(jreq[JKEY_ODM], JKEY_ODM_LEFT, sensor_data.right_code, 0);
  JV_TO_DOUBLE(jreq[JKEY_ODM], JKEY_ODM_V, sensor_data.v, 0);
  JV_TO_DOUBLE(jreq[JKEY_ODM], JKEY_ODM_W, sensor_data.w, 0);
  JV_TO_INT(jreq[JKEY_COLLISION], JKEY_COLLISION_LF, sensor_data.lf_bump, 0);
  JV_TO_INT(jreq[JKEY_COLLISION], JKEY_COLLISION_LB, sensor_data.lb_bump, 0);
  JV_TO_INT(jreq[JKEY_COLLISION], JKEY_COLLISION_RF, sensor_data.rf_bump, 0);
  JV_TO_INT(jreq[JKEY_COLLISION], JKEY_COLLISION_RB, sensor_data.rb_bump, 0);
  JV_TO_INT(jreq[JKEY_ALARM], JKEY_ABNORMAL, sensor_data.alarm, 0);
  if (jreq[JKEY_VL53L5CX][JKEY_DATA].isArray()) {
    int data_size = jreq[JKEY_VL53L5CX][JKEY_DATA].size();
    for (int i = 0; i < data_size; ++i) {
      sensor_data.tof_dis.emplace_back((uint16)jreq[JKEY_VL53L5CX][JKEY_DATA][i].asInt());
    }
  }

  c_reply_->async_call("send_sensor_data", sensor_data);

  return 0;
}

int RpcManager::SendAstarPlan(Json::Value &jreq, Json::Value &jret) {
  if (c_reply_ == nullptr || c_reply_->get_connection_state() != rpc::client::connection_state::connected) {
    DLOG_WARNING(MOD_BUS, "---------rpc connection lost. SendAstarPlan----------");
    return 0;
  }
  RpcPath astar_plan;
  if (jreq[JKEY_ASTAR].isArray()) {
    RpcPose pose;
    int path_size = jreq[JKEY_ASTAR].size();
    for (int i = 0; i < path_size; ++i) {
      pose.x = jreq[JKEY_ASTAR][i][0].asFloat();
      pose.y = jreq[JKEY_ASTAR][i][1].asFloat();
      astar_plan.path.emplace_back(pose);
    }
  }
  c_reply_->async_call("send_astar_plan", astar_plan);

  return 0;
}

int RpcManager::SendCoverPlan(Json::Value &jreq, Json::Value &jret) {
  if (c_reply_ == nullptr || c_reply_->get_connection_state() != rpc::client::connection_state::connected) {
    DLOG_WARNING(MOD_BUS, "---------rpc connection lost. SendCoverPlan----------");
    return 0;
  }
  RpcPath cover_plan;
  if (jreq[JKEY_COVER].isArray()) {
    RpcPose pose;
    int path_size = jreq[JKEY_COVER].size();
    for (int i = 0; i < path_size; ++i) {
      pose.x = jreq[JKEY_COVER][i][0].asFloat();
      pose.y = jreq[JKEY_COVER][i][1].asFloat();
      cover_plan.path.emplace_back(pose);
    }
  }
  c_reply_->async_call("send_cover_plan", cover_plan);

  return 0;
}

}