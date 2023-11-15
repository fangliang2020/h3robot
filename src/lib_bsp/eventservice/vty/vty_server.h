#ifndef EVENTSERVICE_VTY_VTYSERVER_H_
#define EVENTSERVICE_VTY_VTYSERVER_H_

#include <list>

#include "eventservice/base/basictypes.h"
#include "eventservice/base/sigslot.h"
#include "eventservice/base/criticalsection.h"
#include "eventservice/log/log_client.h"
#include "boost/boost/boost_settings.hpp"
#include "eventservice/net/eventservice.h"
#include "eventservice/net/networktinterface.h"
#include "eventservice/vty/vty_client.h"
#include "eventservice/vty/vty_common.h"
#include "eventservice/vty/vty_processor.h"

namespace vty {

class VtyServer: public boost::noncopyable,
  public sigslot::has_slots<>,
  public boost::enable_shared_from_this<VtyServer> {
 public:
  typedef boost::shared_ptr<VtyServer> Ptr;
  VtyServer() {
    close_flag_ = false;

  }
  ~VtyServer() {
  }

 public:
  bool Start(const chml::SocketAddress &address);
  bool Reset(const chml::SocketAddress &address);
  void Close();
  bool IsClose();
  int RegistCmd(const char *cmd, VtyCmdHandler handler,
                const void *user_data, const char *info = VTY_CMD_DEFAULT_INFO);
  int DeregistCmd(const char *cmd);

 private:
  void OnListenerAcceptEvent(chml::AsyncListener::Ptr listener, chml::Socket::Ptr s, int err);
  void OnSocketErrorEvent(VtyProcessor::Ptr proc, int err);
  void OnVtyDisconnectEvent(VtyProcessor::Ptr proc, int err);

 private:
  chml::CriticalSection     crit_;
  bool close_flag_;
  chml::SocketAddress address_;
  chml::EventService::Ptr server_es_;
  chml::AsyncListener::Ptr async_listener_;
  std::list<VtyProcessor::Ptr> proc_vtys_;
  VtyCmdList<VtyCmdHandler>::Ptr cmds_;
};

}  // namespace vty

#endif  // EVENTSERVICE_VTY_VTYSERVER_H_
