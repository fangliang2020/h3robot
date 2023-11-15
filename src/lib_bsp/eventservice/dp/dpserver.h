#ifndef EVENTSERVICE_DP_DPSERVER_H_
#define EVENTSERVICE_DP_DPSERVER_H_

#include "eventservice/net/eventservice.h"

#include "eventservice/dp/dpagent.h"
#include "eventservice/dp/dpclient.h"

namespace chml {

class DpServer : public boost::noncopyable,
  public boost::enable_shared_from_this<DpServer>,
  public sigslot::has_slots<> {
 public:
  typedef boost::shared_ptr<DpServer> Ptr;

 public:
  DpServer(EventService::Ptr es = EventService::Ptr());
  ~DpServer();

  // Start the service.
  bool Start(const SocketAddress addr);
  // Reset the local IP address.
  bool Reset(const SocketAddress addr);

 protected:
  void OnListenerAcceptEvent(AsyncListener::Ptr listener, Socket::Ptr s, int err);
  void OnAgentErrorEvent(DpAgent::Ptr dp_agent, int err);

 private:
  EventService::Ptr    event_service_;
  SocketAddress        address_;
  AsyncListener::Ptr   async_listener_;

 private:
  typedef std::list<DpAgent::Ptr> DP_CLI_LIST;
  DP_CLI_LIST                     dp_agents_;
};
}  // namespace chml

#endif  // EVENTSERVICE_DP_DPSERVER_H_
