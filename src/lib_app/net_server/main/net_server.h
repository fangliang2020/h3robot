/**
 * @Description: net server
 * @Author:      zhz
 * @Version:     1
 */

#ifndef SRC_LIB_NET_SERVER_H_
#define SRC_LIB_NET_SERVER_H_

#include "core_include.h"
#include "app/app/appinterface.h"
#include "net_server/base/base_module.h"

namespace net {

class NetServer : public app::AppInterface,
                  public boost::enable_shared_from_this<NetServer>,
                  public sigslot::has_slots<> {
 public:
  typedef boost::shared_ptr<NetServer> Ptr;
  typedef std::vector<BaseModule*> VecBaseMdule;

  NetServer();
  virtual ~NetServer();

  bool PreInit(chml::EventService::Ptr event_service);

  bool InitApp(chml::EventService::Ptr event_service);

  bool RunAPP(chml::EventService::Ptr event_service);

  void OnExitApp(chml::EventService::Ptr event_service);

  chml::DpClient::Ptr GetDpClientPtr();
  chml::EventService::Ptr GetEventServicePtr();

 private:
  void OnDpMessage(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg);
  bool CreatBaseNode();
  bool InitBaseNode();

 private:
  VecBaseMdule base_node_;
  chml::DpClient::Ptr dp_client_;
  chml::EventService::Ptr event_main_;
};

}

#endif  // SRC_LIB_NET_SERVER_H_