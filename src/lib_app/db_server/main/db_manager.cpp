/**
 * @Description: db server
 * @Author:      fl
 * @Version:     1
 */
#include "db_manager.h"
#include "db_server/base/jkey_common.h"
#include "db_server/db_peripheral/db_peripheral.h"
#include "db_server/db_net/db_network.h"
#include "../../lib_bsp/libmldb/mldb.h"


#define MSG_DB_TIMING 0x109

namespace db {

DbServer::DbServer()
  : AppInterface("db_server"){
  YamlConf=new YamlConfig;
  flag_dbexit=false;
}

DbServer::~DbServer(){
  for(auto &node : base_node_){
    if(node) {
      node->Stop();
      delete node;
      node=nullptr;
    }
  }
  base_node_.clear();
  delete YamlConf;
}

bool DbServer::CreateBaseNode() {
  
  base_node_.emplace_back(new Db_Network(this,YamlConf));
  base_node_.emplace_back(new Db_Peripheral(this,YamlConf));
  for(auto &node : base_node_){
    if(node == nullptr) {
        DLOG_ERROR(MOD_EB,"dbserver create object failed.");
        return false;
    }
  }
  return true;
}

bool DbServer::InitBaseNode(){
  for(auto &node : base_node_){
    if(node){
      node->Start();
    }
  }
  return true;
}

bool DbServer::PreInit(chml::EventService::Ptr event_service) {
  Event_Service_=event_service;
  chml::SocketAddress dp_db_addr(ML_DP_SERVER_HOST, ML_DP_SERVER_PORT);
  dp_client_=chml::DpClient::CreateDpClient(event_service,dp_db_addr,"dp_db_server");
  ASSERT_RETURN_FAILURE(!dp_client_, false);
  dp_client_->SignalDpMessage.connect(this,&DbServer::OnDpMessage);
  // if (access(DBPATH, 0)) {
  //     DLOG_ERROR(MOD_EB,"dbmanager: %s folder does not exist\n", DBPATH);
  //     flag_dbexit=true;
  // }
  // if(flag_dbexit){
  mldb_init(DBPATH);
  // }
  YamlConf->init(YAMLPATH);
  return CreateBaseNode();//创建对象并检查
}

bool DbServer::InitApp(chml::EventService::Ptr event_service) {
  return InitBaseNode();
}
bool DbServer::RunAPP(chml::EventService::Ptr event_service) {
  //接收到消息后需要回复request,相当于注册一个 "DP_DB_REQUEST"服务，都可以被其他进程调用
  if (dp_client_->ListenMessage("DP_DB_REQUEST", chml::TYPE_REQUEST)) {
    DLOG_ERROR(MOD_EB, "dp client listen message failed.");
  }

  return true;
}

void DbServer::OnExitApp(chml::EventService::Ptr event_service) {

}

chml::DpClient::Ptr DbServer::GetDpClientPtr() {
  return dp_client_;
}

chml::EventService::Ptr DbServer::GetEventServicePtr() {
  return Event_Service_;
}

void DbServer::OnDpMessage(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg) {
  int ret=-1;
  std::string stype;
  Json::Value jreq,jret;
  DLOG_DEBUG(MOD_EB,"=== dp %s",dp_msg->req_buffer.c_str());
  if(dp_msg->method=="DP_DB_REQUEST") {
    if (!String2Json(dp_msg->req_buffer, jreq)) {
      DLOG_ERROR(MOD_EB, "json parse failed.");
      return;
    }
    stype = jreq[JKEY_TYPE].asString();
    DLOG_INFO(MOD_EB,"type==== %s",stype.c_str());
    for (auto &node : base_node_) {
      ret = node->OnDpMessage(stype, jreq, jret); 
      //遍历每个子类的OnDpMessage
      if (-1 != ret) {
        break;
      }
    }
  } 
  if(dp_msg->type == chml::TYPE_REQUEST) {
    jret[JKEY_TYPE]=stype;
    if(ret==0) {
      jret[JKEY_STATE]=200;
      jret[JKEY_ERR]="success";
    }
    else if(ret==2){
      jret[JKEY_STATE]=400;
      jret[JKEY_ERR]="error";
    }
    else {

    }
    Json2String(jret, *dp_msg->res_buffer);
    DLOG_INFO(MOD_EB, "ret %s", (*dp_msg->res_buffer).c_str());
    dp_cli->SendDpReply(dp_msg);
  }
}
}

