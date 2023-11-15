#include <iostream>
#include <stdio.h>
#include "app/app/app.h"
#include "eventservice/dp/dpclient.h"
#include "eventservice/dp/dpserver.h"
#include "eventservice/net/eventservice.h"
#include "dp_alg_server.h"
#include "dp_method.h"

#undef ML_DP_SERVER_HOST
#define ML_DP_SERVER_HOST "127.0.0.1"
//#define ML_DP_SERVER_HOST "0.0.0.0"

#define ML_MCAST_HOST "228.5.6.2"
#define ML_MCAST_PORT 22100
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

  
void OnUdpErrorEvent( chml::AsyncUdpSocket::Ptr udp_socket, int err) {
  DLOG_ERROR(MOD_DG, "CDpCliTest udp socket error! error_code: %d! "
             "Restart after 5s!", err);
  //event_services_->PostDelayed(5000, this, MSG_RESTART);
}

  
void OnUdpSocketWrite(chml::AsyncUdpSocket::Ptr udp_socket) {
  //TryToSendMulticastMessage();
  DLOG_INFO(MOD_DG, "OnUdpSocketWrite");
}

void OnUdpSocketRead(chml::AsyncUdpSocket::Ptr udp_socket,
                                 chml::MemBuffer::Ptr buffer,
                                 const chml::SocketAddress &addr)
{
  char buff[1400];

  std::size_t buffer_size = buffer->size() > 1400 ? 1400 : buffer->size();
  buffer->ReadBytes(buff, buffer_size);
  DLOG_INFO(MOD_DG, "OnUdpSocketRead %d addr %s\n", buffer_size, addr.ToString().c_str());
  mcast_sock_->AsyncRead();
}
virtual bool PreInit(chml::EventService::Ptr event_service)
{
  event_service_ = event_service;
  if (nullptr == event_service_)
  {
    event_service_ = chml::EventService::CreateEventService();
  }


  chml::SocketAddress addr(ML_MCAST_HOST, ML_MCAST_PORT);
  mcast_sock_ = event_service_->CreateMulticastSocket(addr, "eth0");
  if (mcast_sock_) {
    mcast_sock_->SignalSocketErrorEvent.connect(this, &CDpCliTest::OnUdpErrorEvent);
    mcast_sock_->SignalSocketReadEvent.connect(this, &CDpCliTest::OnUdpSocketRead);
    mcast_sock_->SignalSocketWriteEvent.connect(this, &CDpCliTest::OnUdpSocketWrite);
    mcast_sock_->AsyncRead();
  } else {
    DLOG_ERROR(MOD_DG, "CreateMulticastSocket failed");
  }

  chml::SocketAddress saddr(ML_DP_SERVER_HOST, ML_DP_SERVER_PORT);
  dp_client_ = chml::DpClient::CreateDpClient(event_service_, saddr, "dp_test");
  ASSERT_RETURN_FAILURE((nullptr == dp_client_), false);

  dp_client_->SignalDpMessage.connect(this, &CDpCliTest::OnDpMessage);

  if (is_server_)
  {
    dp_client_->ListenMessage("HELLO", chml::TYPE_MESSAGE);
  }
  else
  {
    event_service_->PostDelayed(1 * 1000, this);
  }
  return true;
}
virtual bool InitApp(chml::EventService::Ptr event_service)
{
  return true;
}
virtual bool RunAPP(chml::EventService::Ptr event_service)
{
  return true;
}
virtual void OnExitApp(chml::EventService::Ptr event_service)
{
}

void OnDpMessage(chml::DpClient::Ptr dp_client, chml::DpMessage::Ptr dp_msg)
{
  
}

void OnMessage(chml::Message *msg)
{
  if (!mcast_sock_) {
    return;
  }
  chml::SocketAddress addr(ML_MCAST_HOST, ML_MCAST_PORT);
  mcast_sock_->SendTo("wahaha", 6, addr);
  event_service_->PostDelayed(2 * 1000, this);
}

private:
chml::EventService::Ptr event_service_;
chml::DpClient::Ptr dp_client_;

private:
bool is_server_;
chml::AsyncUdpSocket::Ptr mcast_sock_;
};

int main(int argc, char *argv[]) {
  bool bret = false;
  printf("dp clinet compile %s %s\n", __DATE__, __TIME__);

#if 1
  bret = Log_Init(true);
  ASSERT_RETURN_FAILURE((false == bret), -1);
#else
  bret = Log_ClientInit();
  ASSERT_RETURN_FAILURE((false == bret), -2);
#endif
  sys_log_init();
  set_syslog_level(8);

  Log_SetSyncPrint(true);
  Log_DbgSetLevel(MOD_EB, LL_DEBUG);
  chml::EventService::Ptr dp_srv =
    chml::EventService::CreateCurrentEventService("dp_server");
  ASSERT_RETURN_FAILURE((NULL == dp_srv), -5);


  CDpCliTest srv(true);
 
  srv.PreInit(dp_srv);
  srv.InitApp(dp_srv);
  srv.RunAPP(dp_srv);

  while (true) {
    dp_srv->Run();
  }

  //Vty_Deinit();
  return EXIT_SUCCESS;
}
