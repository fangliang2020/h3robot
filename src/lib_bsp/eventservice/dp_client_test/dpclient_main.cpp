#include <iostream>
#include <stdio.h>
#include "app/app/app.h"
#include "eventservice/dp/dpclient.h"
#include "eventservice/dp/dpserver.h"
#include "eventservice/net/eventservice.h"

#undef ML_DP_SERVER_HOST
#define ML_DP_SERVER_HOST "192.168.0.60"
//#define ML_DP_SERVER_HOST "0.0.0.0"

//#undef ML_DP_SERVER_PORT
//#define ML_DP_SERVER_PORT 0

class CDpCliTest : public app::AppInterface,
  public chml::MessageHandler,
  public sigslot::has_slots<> {
 public:
  CDpCliTest(bool is_srv) 
    : app::AppInterface("dp_client")
    , is_server_(is_srv) {
  }
  ~CDpCliTest() {
  }

  virtual bool PreInit(chml::EventService::Ptr event_service) {
    event_service_ = event_service;
    if (nullptr == event_service_) {
      event_service_ = chml::EventService::CreateEventService();
    }

    chml::SocketAddress saddr(ML_DP_SERVER_HOST, ML_DP_SERVER_PORT);
    dp_client_ = chml::DpClient::CreateDpClient(event_service_, saddr, "dp_test");
    ASSERT_RETURN_FAILURE((nullptr == dp_client_), false);

    dp_client_->SignalDpMessage.connect(this, &CDpCliTest::OnDpMessage);

    if (is_server_) {
      dp_client_->ListenMessage("DP_DB_BROADCAST", chml::TYPE_MESSAGE);
    } else {
      event_service_->PostDelayed(1*1000, this);
    }
    return true;
  }
  virtual bool InitApp(chml::EventService::Ptr event_service) {
    return true;
  }
  virtual bool RunAPP(chml::EventService::Ptr event_service) {
    return true;
  }
  virtual void OnExitApp(chml::EventService::Ptr event_service) {
  }

  void OnDpMessage(chml::DpClient::Ptr dp_client, chml::DpMessage::Ptr dp_msg) {
    printf("----- OnDpMessage %s.\n", chml::XTimeUtil::TimeLocalToString().c_str());
    *dp_msg->res_buffer = "worlds";
    printf("--------req_buffer %s-----\n", dp_msg->req_buffer.c_str());
    if (chml::TYPE_REQUEST == dp_msg->type) {
      dp_client->SendDpReply(dp_msg);
    }
  }

  void OnMessage(chml::Message* msg) {
#if 1
    std::string sresp;
    dp_client_->SendDpMessage("HELLO", 0, "hello", 5, &sresp);
    if (!sresp.empty()) {
      printf("--------------- recv %s.\n", sresp.c_str());
    }
#else
    std::string sreq = R"test(
      {
        "type" : "AVS_TRIGGER",
          "body" : {
            "trigger_result" : 1,
            "trigger_type" : 3
          }
      }
    )test";
    std::string sresp;
    int ret = dp_client_->SendDpMessage(ALG_REQUEST_MESSAGE, 0, sreq.c_str(), sreq.size(), &sresp);
    printf("-----------%d-----------%s.\n", ret,
           chml::XTimeUtil::TimeMSLocalToString().c_str());
#endif
    event_service_->PostDelayed(2*1000, this);
  }

 private:
  chml::EventService::Ptr event_service_;
  chml::DpClient::Ptr     dp_client_;

 private:
  bool is_server_;
};

int main(int argc, char *argv[]) {
  bool bret = false;
  printf("dp clinet compile %s %s\n", __DATE__, __TIME__);

  bret = Log_ClientInit();
  ASSERT_RETURN_FAILURE((false == bret), -2);

  chml::EventService::Ptr dp_srv =
    chml::EventService::CreateCurrentEventService("dp_server");
  ASSERT_RETURN_FAILURE((NULL == dp_srv), -5);

  CDpCliTest srv(true);
  CDpCliTest cli(false);
  if (argc == 1) {
    srv.PreInit(dp_srv);
    srv.InitApp(dp_srv);
    srv.RunAPP(dp_srv);

    // chml::EventService::Ptr dp_cli =
    //   chml::EventService::CreateEventService(NULL, "dp_client");
    // ASSERT_RETURN_FAILURE((NULL == dp_srv), -5);
    // cli.PreInit(dp_cli);
    // cli.InitApp(dp_cli);
    // cli.RunAPP(dp_cli);
  } else {
    chml::EventService::Ptr dp_cli =
      chml::EventService::CreateEventService(NULL, "dp_client");
    ASSERT_RETURN_FAILURE((NULL == dp_srv), -5);
    cli.PreInit(dp_cli);
    cli.InitApp(dp_cli);
    cli.RunAPP(dp_cli);
  }
  
  while (true) {
    dp_srv->Run();
  }

  //Vty_Deinit();
  return EXIT_SUCCESS;
}
