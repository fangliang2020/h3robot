/**
 * @Description: rpc pc调试
 * @Author:      zhz
 * @Version:     1
 */

#ifndef SRC_LIB_RPC_MANAGER_H_
#define SRC_LIB_RPC_MANAGER_H_

#include "rpc/client.h"
#include "rpc/server.h"
#include "net_server/base/base_module.h"

namespace net {

struct RpcMap {
  int width;
  int height;
  float resolution;
  float orig[3];
  std::vector<int8_t> map;

  RpcMap()
    : width(0),
      height(0),
      resolution(0) {
    orig[0] = 0;
    orig[1] = 0;
    orig[2] = 0;
  }
  
  MSGPACK_DEFINE_MAP(width, height, resolution, orig, map)
};

struct RpcPose {
  float x;
  float y;

  RpcPose()
    : x(0),
      y(0) {
  }

  MSGPACK_DEFINE_ARRAY(x, y)
};

struct RpcTrajPose {
  float x;
  float y;
  float theta;

  RpcTrajPose()
    : x(0),
      y(0),
      theta(0) {
  }

  MSGPACK_DEFINE_ARRAY(x, y, theta)
};

struct RpcAssetTraj {
  std::vector<RpcTrajPose> traj;
  MSGPACK_DEFINE_ARRAY(traj)
};

struct RpcState {
  std::string state;
  MSGPACK_DEFINE_ARRAY(state)
};

struct RpcPoseLaser {
  std::vector<RpcPose> laser;
  MSGPACK_DEFINE_MAP(laser)
};

struct RpcPath {
  std::vector<RpcPose> path;
  MSGPACK_DEFINE_ARRAY(path)
};

struct RpcSensorData {
  long long tv_sec;
  long long tv_nsec;
  uint8_t status;      // 机器当前状态
  uint8_t battery;     // 电池电量
  uint8_t dis;         // 沿边距离值
  uint8_t alarm;       // 报警信号
  float x_accel;       // x加速度
  float y_accel;       // y加速度
  float z_accel;       // z加速度
  float x_gyro;        // x角速度
  float y_gyro;        // y角速度
  float z_gyro;        // z角速度
  float course_angle;  // 航向角度
  int left_code;       // 左码盘值
  int right_code;      // 右码盘值
  uint8_t lf_bump;     // 左前撞板
  uint8_t lb_bump;     // 左后撞板
  uint8_t rf_bump;     // 右前撞板
  uint8_t rb_bump;     // 右后撞板
  uint16_t lf_gnd;     // 左前地检
  uint16_t lb_gnd;     // 左后地检
  uint16_t rf_gnd;     // 右前地检
  uint16_t rb_gnd;     // 右后地检
  float left_wheel;    // 左轮电机电流值
  float right_wheel;   // 右轮电机电流值
  float fan_motor;     // 风机电流值
  float mid_motor;     // 中扫电流值
  float left_side;     // 左边扫电流值
  float right_side;    // 右边扫电流值
  float mop_motor;     // 拖布电机电流值
  float v;             // 正向速度
  float w;             // 旋转弧度
  std::vector<uint16> tof_dis;

  RpcSensorData()
    : tv_sec(0),
      tv_nsec(0),
      status(0),
      battery(0), 
      dis(0),
      alarm(0),
      x_accel(0),
      y_accel(0), 
      z_accel(0),
      x_gyro(0),
      y_gyro(0),
      z_gyro(0), 
      course_angle(0),
      left_code(0),
      right_code(0),
      lf_bump(0), 
      lb_bump(0),
      rf_bump(0), 
      rb_bump(0),
      lf_gnd(0),
      lb_gnd(0),
      rf_gnd(0), 
      rb_gnd(0),
      left_wheel(0),
      right_wheel(0),
      fan_motor(0), 
      mid_motor(0),
      left_side(0),
      right_side(0),
      mop_motor(0), 
      v(0),
      w(0) {
  }
  
  MSGPACK_DEFINE_MAP(tv_sec, tv_nsec, status, battery, dis, alarm, x_accel, y_accel, z_accel, 
    x_gyro, y_gyro, z_gyro, course_angle, left_code, right_code, lf_bump, lb_bump, rf_bump, rb_bump, 
    lf_gnd, lb_gnd, rf_gnd, rb_gnd, left_wheel, right_wheel, fan_motor, mid_motor, left_side, right_side,
    mop_motor, v, w, tof_dis)
};

class RpcManager : public BaseModule {
 public:
  explicit RpcManager(NetServer *net_server_ptr);
  virtual ~RpcManager();

  int Start();
  int Stop();
  int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
 
 public:
  // rpc client
  int OdmSensorData(Json::Value &jreq, Json::Value &jret);
  int SendSensorData(Json::Value &jreq, Json::Value &jret);
  int SendNowState(Json::Value &jreq, Json::Value &jret);
  int SendPoseLaser(Json::Value &jreq, Json::Value &jret);
  int SendAstarPlan(Json::Value &jreq, Json::Value &jret);
  int SendCoverPlan(Json::Value &jreq, Json::Value &jret);

 private:
  static void RunRpcServer();
  // rpc server
  static bool Connect(int ip_a, int ip_b, int ip_c, int ip_d);
  static RpcMap GetMap();
  static RpcAssetTraj GetTraj(int size);
  static void ClearTraj();
  static void CmdGoForward();
  static void CmdGoBackward();
  static void CmdTurnLeft();
  static void CmdTurnRight();
  static void CmdMoveStop();
  static void CmdPause();
  static void CmdResume();
  static void CmdStop();
  static void CmdGlobalClean();
  static void CmdSetFanGear(int gear);
  static void CmdSetMidGear(int gear);
  static void CmdSetSideGear(int gear);
  static void CmdMopUp();
  static void CmdMopDown();
  static void CmdToCharge();
  static void CmdToWash();
  static void CmdLeftTofCalibrate();
  static void CmdMidTofCalibrate();
  static void CmdRightTofCalibrate();
  static void CmdFrontTofDemarcate();
  static void CmdBorderCalibrate();
  static void CmdBorderDemarcate();
  static void CmdImuCalibrate();
  static void CmdOdometerCalibrate();
  static void CmdGndCalibrate();

 private:
  static chml::DpClient::Ptr dp_client_;
  static rpc::client* c_reply_;
  static chml::CriticalSection c_mutex_;
  static bool recv_connect_; // 收到PC的connect命令后才报状态
  static int status_;
};

}

#endif  // SRC_LIB_RPC_MANAGER_H_