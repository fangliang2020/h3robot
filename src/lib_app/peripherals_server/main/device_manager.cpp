/**
 * @Description: peripherals server
 * @Author:      fl
 * @Version:     1
 */

#include "device_manager.h"
#include "peripherals_server/base/jkey_common.h"
#include "peripherals_server/controls/gpio_manager.h"
#include "peripherals_server/controls/ir_sensor.h"
#include "peripherals_server/controls/misc_manager.h"
#include "peripherals_server/controls/motor_manager.h"
#include "peripherals_server/controls/wheel_manager.h"
#include "peripherals_server/imu/imu_sensor.h"
#include "peripherals_server/vl53l5cx/vl53l5cx_sensor.h"
#include "peripherals_server/vl6180x/vl6180x_sensor.h"
#include "peripherals_server/adc/adc_manager.h"
#include "peripherals_server/audio/voice_module.h"

#define MSG_SENSOR_TIMING 0x106

namespace peripherals{

PerServer::PerServer()
  : AppInterface("peripherals_server"){

}

PerServer::~PerServer(){
  for(auto &node : base_node_){
    if(node){
      node->Stop();
      delete node;
      node=nullptr;
    }
  }
  base_node_.clear();
}

bool PerServer::CreateBaseNode(){
  base_node_.emplace_back(new GpioManager(this));
  base_node_.emplace_back(new MotorManager(this));
  base_node_.emplace_back(new WheelManager(this));
  base_node_.emplace_back(new Vl6180xManager(this));
  base_node_.emplace_back(new Vl53l5cxManager(this));
  base_node_.emplace_back(new AdcManager(this));
  base_node_.emplace_back(new ImuManager(this));
  base_node_.emplace_back(new Ir_Sensor_Dev(this));
  for(auto &node : base_node_){
    if(node == nullptr){
        printf("peripherals create object failed.");
        return false;
    }
  }
  return true;
}

bool PerServer::InitBaseNode(){
  for(auto &node : base_node_){
    if(node){
      node->Start();
    }
  }
  return true;
}

bool PerServer::PreInit(chml::EventService::Ptr event_service) {
  // VoicePlaySelf(PLAY_Q001); ///放音后沿边会中断
  Event_Sensor_=chml::EventService::CreateEventService(NULL, "Sensor_dp");
  //接收用dp_client_rev
  chml::SocketAddress dp_per_addr(ML_DP_SERVER_HOST, ML_DP_SERVER_PORT);
  dp_client_rev=chml::DpClient::CreateDpClient(event_service,dp_per_addr,"dp_peripherals_rev");
  ASSERT_RETURN_FAILURE(!dp_client_rev, false);
  dp_client_rev->SignalDpMessage.connect(this,&PerServer::OnDpMessage);
  //发送用 dp_client_send
  dp_client_send=chml::DpClient::CreateDpClient(Event_Sensor_,dp_per_addr,"dp_peripherals_send");
  ASSERT_RETURN_FAILURE(!dp_client_send, false);

  // Event_Sensor_->PostDelayed(100, this, MSG_SENSOR_TIMING);

  MyEvent_Service_=event_service;
  return CreateBaseNode();//创建对象并检查
}

bool PerServer::InitApp(chml::EventService::Ptr event_service) {
  return InitBaseNode();//调用start运行
}

bool PerServer::RunAPP(chml::EventService::Ptr event_service) {
  //接收的广播消息，可公用，无响应
  if (dp_client_rev->ListenMessage("DP_SCHEDULE_BROADCAST", chml::TYPE_MESSAGE)) {
    DLOG_ERROR(MOD_EB, "dp client listen message failed.");
  }
  //接收到消息后需要回复request,相当于注册一个 "DP_PERIPHERALS_REQUEST"服务，都可以被其他进程调用
  if (dp_client_rev->ListenMessage("DP_PERIPHERALS_REQUEST", chml::TYPE_REQUEST)) {
    DLOG_ERROR(MOD_EB, "dp client listen message failed.");
  }
  
  VoicePlaySelf(PLAY_Q003);
  return true;
}

void PerServer::OnExitApp(chml::EventService::Ptr event_service) {
}

chml::DpClient::Ptr PerServer::GetDpClientPtr() {
  return dp_client_send;
}

chml::EventService::Ptr PerServer::GetEventServicePtr() {
  return MyEvent_Service_;
}

void PerServer::VoicePlaySelf(const char *tts)
{
  VoiceModule::getVoiceModule()->VoicePlaytts(1,tts);
 // system("pwd");
}

void PerServer::OnDpMessage(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg) {
  int ret=-1;
  std::string stype;
  Json::Value jreq,jret;
  DLOG_INFO(MOD_EB,"=== dp %s",dp_msg->req_buffer.c_str());
  if(dp_msg->method=="DP_SCHEDULE_BROADCAST" || dp_msg->method=="DP_PERIPHERALS_REQUEST") {
    if (!String2Json(dp_msg->req_buffer, jreq)) {
      DLOG_ERROR(MOD_EB, "json parse failed.");
      return;
    }
    stype = jreq[JKEY_TYPE].asString();
    DLOG_INFO(MOD_EB,"type==== %s",stype.c_str());
    // if(stype=="set_voice")
    // {
    //   DLOG_INFO(MOD_EB, "play voice\n");
    //   VoicePlaySelf(jreq["body"]["voice_tts"].asString().c_str());
    //   ret=0;
    // }
    // else {
      for (auto &node : base_node_) {
        ret = node->OnDpMessage(stype, jreq, jret); 
        //遍历每个子类的OnDpMessage
        if (-1 != ret) {
          break;
        }
      }
    // }
    if(dp_msg->type == chml::TYPE_REQUEST) {
      jret[JKEY_TYPE]=stype;
      if(ret==0) {
        jret[JKEY_STATUS]=200;
        jret[JKEY_ERR]="success";
      }
      else {
        // jret[JKEY_STATUS]=400;
        // jret[JKEY_ERR]="error";
      }
      Json2String(jret, *dp_msg->res_buffer);
      DLOG_INFO(MOD_EB, "ret %s", (*dp_msg->res_buffer).c_str());
      dp_cli->SendDpReply(dp_msg);
    }
  } 
}

void PerServer::OnMessage(chml::Message* msg) {
	if(MSG_SENSOR_TIMING == msg->message_id) {
      SensorBroadcast();
    Event_Sensor_->PostDelayed(20, this, MSG_SENSOR_TIMING);
	//	Event_Sensor_->Post(this,MSG_SENSOR_TIMING);
	}
}

void PerServer::SensorBroadcast() {
    std::string str_req,str_res;
    Json::Value json_req;
    uint32_t battery;
    /*条件变量等待*/
    // std::unique_lock<std::mutex> lock(tex);
    // cv.wait(lock, []{return ready;});
    // if(ImuRdflag) {

      json_req[JKEY_TYPE] = "sensor_data";
      //vl53l5cx数据
      Json::Value& json_data = json_req[JKEY_BODY]["vl53l5cx"]["data"];
      for(int i=0;i<192;i++) {
        json_data.append(vl53_dis[i]);
      }
      //vl6180数据
      json_req[JKEY_BODY]["vl6180"]["dis"] = vl6180_dis;
      //imu数据
      Json::Value& json_imu = json_req[JKEY_BODY]["imu"];
      json_imu["x_accel"] = ImuReal.accel_x_act;
      json_imu["y_accel"] = ImuReal.accel_y_act;
      json_imu["z_accel"] = ImuReal.accel_z_act;
      json_imu["x_gyro"] = ImuReal.gyro_x_act;
      json_imu["y_gyro"] = ImuReal.gyro_y_act;
      json_imu["z_gyro"] = ImuReal.gyro_z_act;
      json_imu["course_angle"] = ImuReal.course_angle;
      //超声波地毯检测数据
      json_req[JKEY_BODY]["ultra"]["rug"] = 1;
      //电机电流检测数据
      Json::Value& json_curr = json_req[JKEY_BODY]["current"];
      json_curr["wheel_l_curr"] = LeftWheel.M_Prot.currentValue;
      json_curr["wheel_r_curr"] = RightWheel.M_Prot.currentValue;
      json_curr["fan_curr"] = Motorfan.M_Prot.currentValue;
      json_curr["mid_curr"] = Motormid.M_Prot.currentValue;
      json_curr["side_l_curr"] = MotorsideA.M_Prot.currentValue;
      json_curr["side_r_curr"] = MotorsideB.M_Prot.currentValue;
      json_curr["mop_curr"] = MotorMop.M_Prot.currentValue;
      //电量数据
      if(VBAT_Prot.currentValue<12.5f) {
        battery=0;
      }
      else if(VBAT_Prot.currentValue > 16.5f) {
        battery=100;
      }
      else {
        battery=(VBAT_Prot.currentValue-12.5)*100/4;
      }
      json_req[JKEY_BODY]["battery"]["electric_qy"] =battery;
      //地检检测数据
      Json::Value& json_gnddet = json_req[JKEY_BODY]["gnd-det"];
      json_gnddet["gnddet_lf"] = GND_Sensor_LF.readVal;
      json_gnddet["gnddet_lb"] = GND_Sensor_LB.readVal;
      json_gnddet["gnddet_rf"] = GND_Sensor_RF.readVal;
      json_gnddet["gnddet_rb"] = GND_Sensor_RB.readVal; 
      //地检检测数据
      Json::Value& json_odm = json_req[JKEY_BODY]["odm"];
      json_odm["odm_left"] = gODM_CAL.leftv;
      json_odm["odm_right"] = gODM_CAL.rightv;
      json_odm["odm_v"] = gODM_CAL.p_vx;
      json_odm["odm_w"] = gODM_CAL.p_vyaw;
      //报警信号
      json_req[JKEY_BODY]["alarm"]["abnormal"] = abnormal;
      //撞板信号
      Json::Value& json_col = json_req[JKEY_BODY]["collision"];
      json_col["collision_lf"] = (Json::Int)Gpio_PinD.at("COLLISION_DET_LA");
      json_col["collision_lb"] = (Json::Int)Gpio_PinD.at("COLLISION_DET_LB");
      json_col["collision_rf"] = (Json::Int)Gpio_PinD.at("COLLISION_DET_RA");
      json_col["collision_rb"] = (Json::Int)Gpio_PinD.at("COLLISION_DET_RB"); 
      //其他霍尔
      Json::Value& json_hall = json_req[JKEY_BODY]["triggle"];
      json_hall["radar_triggle"]= 0;
      json_hall["visor_det"]= 0;
      Json2String(json_req, str_req);
    //  printf("%s\n",str_req.c_str());
      dp_client_send->SendDpMessage("DP_PERIPHERALS_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
      //ready=false;
      ImuRdflag=false;       
    // }
}

}


