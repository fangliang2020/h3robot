#include "kvdb/kvdb/kvdbclient.h"
#include "eventservice/dp/dpclient.h"
#include "eventservice/log/log_client.h"
#include "kvdbserver.h"
#include "jkey_comm.h"
#include "return_define.h"
#include "interface/dp_kvdb_server.h"

#define MSG_REINIT_DP_CLIENT 1
namespace cache {

  KvdbServer::KvdbServer(chml::EventService::Ptr es) {
    main_service_ = es;
  }

  KvdbServer::~KvdbServer() {
  }

  bool KvdbServer::Start() {
    addr_.SetIP(ML_DP_SERVER_HOST);
    addr_.SetPort(ML_DP_SERVER_PORT);
    // wait dp server ready
    main_service_->PostDelayed(10000, this, MSG_REINIT_DP_CLIENT);    
    if (NULL == kvdb_.get()) {
      kvdb_ = cache::KvdbClient::CreateKvdbClient(KVDB_NAME_KVDB);
      if (NULL == kvdb_.get()) {
        DLOG_ERROR(MOD_EB, "Create KvdbClient failed.");
        return false;
      }
    }
    if (NULL == skvdb_.get()) {
      skvdb_ = cache::KvdbClient::CreateKvdbClient(KVDB_NAME_SKVDB);
      if (NULL == skvdb_.get()) {
        DLOG_ERROR(MOD_EB, "Create KvdbClient failed.");
        return false;
      }
    }
    if (NULL == usr_kvdb_.get()) {
      usr_kvdb_ = cache::KvdbClient::CreateKvdbClient(KVDB_NAME_USRKVDB);
      if (NULL == usr_kvdb_.get()) {
        DLOG_ERROR(MOD_EB, "Create KvdbClient failed.");
        return false;
      }
    }
    return true;
  }

  void KvdbServer::InitDPClient() {
    dp_client_ = chml::DpClient::CreateDpClient(main_service_, addr_, "KvdbServerDpClient");
    if (dp_client_) {
      dp_client_->SignalDpMessage.connect(this, &KvdbServer::OnDpMessage);
      dp_client_->SignalErrorEvent.connect(this, &KvdbServer::OnErrorMessage);
      dp_client_->ListenMessage(ML_DP_KVDB_GET, chml::TYPE_REQUEST);
      dp_client_->ListenMessage(ML_DP_KVDB_SET, chml::TYPE_REQUEST);
    }else {
      DLOG_WARNING(MOD_EB, "dp_client ListenMessage:kvdb failed");
      main_service_->PostDelayed(1000, this, MSG_REINIT_DP_CLIENT, NULL);
    }
  }


  void KvdbServer::OnDpMessage(chml::DpClient::Ptr dp_client, chml::DpMessage::Ptr dp_msg) {
    Json::Reader reader;
    Json::Value req, res;
    std::string kvdb_value;
    std::string kvdb_key;
    std::string kvdb_type;

    if (!reader.parse(dp_msg->req_buffer, req)) {
      DLOG_ERROR(MOD_EB, "dp message is not valid json");
      res[JKEY_STATE] = RET_ERR;
      res[JKEY_ERR_MSG] = "json is error";
      Json2String(res, *dp_msg->res_buffer);
      dp_client->SendDpReply(dp_msg);
      return;
    }
    kvdb_key = req[JKEY_KVDB_KEY].asString();
    kvdb_type = req[JKEY_KVDB_TYPE].asString();
    cache::KvdbClient::Ptr op_kvdb_;

    if (kvdb_type == std::string(KVDB_NAME_KVDB))
      op_kvdb_ = kvdb_;
    else if (kvdb_type == std::string(KVDB_NAME_SKVDB))
      op_kvdb_ = skvdb_;
    else if (kvdb_type == std::string(KVDB_NAME_USRKVDB))
      op_kvdb_ = usr_kvdb_;
    else
      op_kvdb_ = kvdb_;

    if (!op_kvdb_) {
      DLOG_ERROR(MOD_EB, "kvdb handle is null");
      res[JKEY_STATE] = RET_ERR;
      res[JKEY_ERR_MSG] = "kvdb is error";      
      Json2String(res, *dp_msg->res_buffer);
      dp_client->SendDpReply(dp_msg);
      return;
    }
    if (dp_msg->method == ML_DP_KVDB_GET){ 
      if (kvdb_key.size() > 0) {
        DLOG_INFO(MOD_EB, "Get kvdb handle key=%s",kvdb_key.c_str());
        op_kvdb_->GetKey(kvdb_key.c_str(), &kvdb_value);
      }   

      if (!(reader.parse(kvdb_value, res))) {
        DLOG_ERROR(MOD_EB, "Get kvdb value is not json value=%s",kvdb_value.c_str());
        res[JKEY_STATE] = RET_ERR;
        res[JKEY_ERR_MSG] = "kvdb is error";      
        dp_client->SendDpReply(dp_msg);
        return;
      }
      
      res[JKEY_STATE] = RET_OK;
      res[JKEY_ERR_MSG] = "All Done";      
      Json2String(res, *dp_msg->res_buffer);
      dp_client->SendDpReply(dp_msg);      
   }else  if (dp_msg->method == ML_DP_KVDB_SET){ 
      Json2String(req[JKEY_KVDB_VALUE], kvdb_value);
     if (kvdb_key.size() > 0 && kvdb_value.size() > 0) { 
        DLOG_INFO(MOD_EB, "Set kvdb handle key=%s val=%s",kvdb_key.c_str(),kvdb_value.c_str());
        op_kvdb_->SetKey(kvdb_key.c_str(),kvdb_value.c_str(),kvdb_value.size());
        res[JKEY_STATE] = RET_OK;
        res[JKEY_ERR_MSG] = "All Done";      
        Json2String(res, *dp_msg->res_buffer);
        dp_client->SendDpReply(dp_msg);
      } else {
        DLOG_ERROR(MOD_EB, "resp json is error.");
        res[JKEY_STATE] = RET_ERR;
        res[JKEY_ERR_MSG] = "kvdb is error";      
        Json2String(res, *dp_msg->res_buffer);
        dp_client->SendDpReply(dp_msg);
      }
    }
  }

 void KvdbServer::OnErrorMessage(chml::DpClient::Ptr dp_client, int err) {
   switch (err) {
     case chml::TYPE_ERROR_DISCONNECTED: {
       DLOG_ERROR(MOD_EB, "Socket disconnect! Error:%d", err);
       break;
     }
     default:
       break;
   }
 }

  void KvdbServer::OnMessage(chml::Message* msg) {
    // register listen dp msg
    if (msg->message_id ==MSG_REINIT_DP_CLIENT ) {
      InitDPClient();
    }
  }
}
