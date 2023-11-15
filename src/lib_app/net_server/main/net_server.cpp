/**
 * @Description: net server
 * @Author:      zhz
 * @Version:     1
 */

#include "net_server.h"
#include "net_server/rpc/rpc_manager.h"
#include "net_server/ntp/ntp_manager.h"
#include "net_server/base/common.h"
#include "net_server/base/jkey_common.h"
#include "net_server/smartp/smartp_manager.h"
// #include "net_server/udp/udp_manager.h"
#include "net_server/mqtt/mqtt_manager.h"
#include "net_server/wifi/wifi_manager.h"

namespace net {

NetServer::NetServer()
  : AppInterface("net_server") {
  dp_client_ = nullptr;
  event_main_ = nullptr;
}

NetServer::~NetServer() {
  for (auto &node : base_node_) {
    if (node) {
      node->Stop();
      delete node;
      node = nullptr;
    }
  }
  base_node_.clear();
}

bool NetServer::CreatBaseNode() {
  // base_node_.emplace_back(new WifiManager(this));
  // base_node_.emplace_back(new UdpManager(this));
  // base_node_.emplace_back(new MqttManager(this));
  base_node_.emplace_back(new SmartpManager(this));
  base_node_.emplace_back(new RpcManager(this));
  base_node_.emplace_back(new NtpManager(this));
  
  for (auto &node : base_node_) {
    if (node == nullptr) {
      printf("netserver create object failed.");
      return false;
    }
  }

  return true;
}

bool NetServer::InitBaseNode() {
  for (auto &node : base_node_) {
    if (node) {
      node->Start();
    }
  }

  return true;
}

bool NetServer::PreInit(chml::EventService::Ptr event_service) {
  event_main_ = event_service;
  chml::SocketAddress dp_srv_addr(ML_DP_SERVER_HOST, ML_DP_SERVER_PORT);
  dp_client_ = chml::DpClient::CreateDpClient(event_service, dp_srv_addr, "dp_net_service");
  ASSERT_RETURN_FAILURE(!dp_client_, false);
  dp_client_->SignalDpMessage.connect(this, &NetServer::OnDpMessage);
  
  return CreatBaseNode();
}

bool NetServer::InitApp(chml::EventService::Ptr event_service) {
  return InitBaseNode();
}

bool NetServer::RunAPP(chml::EventService::Ptr event_service) {
  if (dp_client_->ListenMessage(DP_SCHEDULE_BROADCAST, chml::TYPE_MESSAGE)) {
    DLOG_ERROR(MOD_EB, "dp client listen message failed.");
  }

  if (dp_client_->ListenMessage(DP_PERIPHERALS_BROADCAST, chml::TYPE_MESSAGE)) {
    DLOG_ERROR(MOD_EB, "dp client listen message failed.");
  }
  
  return true;
}

void NetServer::OnExitApp(chml::EventService::Ptr event_service) {
  
}

chml::DpClient::Ptr NetServer::GetDpClientPtr() {
  return dp_client_;
}

chml::EventService::Ptr NetServer::GetEventServicePtr() {
  return event_main_;
}

void NetServer::OnDpMessage(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg) {
  int ret = -1;
  std::string stype;
  Json::Value jreq, jret;

  DLOG_DEBUG(MOD_EB, "=== dp %s\n", dp_msg->req_buffer.c_str());

  if (dp_msg->method == DP_SCHEDULE_BROADCAST || dp_msg->method == DP_PERIPHERALS_BROADCAST) {
    if (!String2Json(dp_msg->req_buffer, jreq)) {
      DLOG_ERROR(MOD_EB, "json parse failed.");
      return;
    }
    stype = jreq[JKEY_TYPE].asString();
    for (auto &node : base_node_) {
      ret = node->OnDpMessage(stype, jreq, jret);
      if (-1 != ret) {
        break;
      }
    }
  }

  if (dp_msg->type == chml::TYPE_REQUEST) {
    // PackResp(jret, stype, ret);
    Json2String(jret, *dp_msg->res_buffer);
    DLOG_INFO(MOD_EB, "ret %s", (*dp_msg->res_buffer).c_str());
    dp_cli->SendDpReply(dp_msg);
  }

  return;
}

}