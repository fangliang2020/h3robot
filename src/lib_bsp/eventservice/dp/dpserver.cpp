#include "eventservice/dp/dpserver.h"
#include "eventservice/net/networktinterfaceimpl.h"

namespace chml {

DpServer::DpServer(EventService::Ptr es)
  : event_service_(es) {
  DLOG_INFO(MOD_EB, "DpServer construct");
}

DpServer::~DpServer() {
  DLOG_INFO(MOD_EB, "DpServer destruct");
  dp_agents_.clear();
  if (event_service_.get()) {
    event_service_.reset();
  }
  if (async_listener_.get()) {
    async_listener_->Close();
    async_listener_.reset();
  }
}

bool DpServer::Reset(const SocketAddress addr) {
  DLOG_KEY(MOD_EB, "Reset DpServer request(new:%s)", addr.ToString().c_str());
  if (addr == address_) {
    DLOG_ERROR(MOD_EB, "Reset DpServer failed, addr same as current");
    return false;
  }
  if (async_listener_.get()) {
    async_listener_->Close();
    async_listener_.reset();
  }
  address_.Clear();
  bool res = Start(addr);
  if (res) {
    DLOG_INFO(MOD_EB, "Reset DpServer successed");
    address_ = addr;
    return true;
  }
  DLOG_ERROR(MOD_EB, "Reset DpServer failed");
  return false;
}

bool DpServer::Start(const SocketAddress addr) {
  bool res = true;
  if (NULL == event_service_.get()) {
    event_service_ = EventService::CreateEventService(NULL, "dp_server");
    ASSERT_RETURN_FAILURE((NULL == event_service_.get()), false);
    event_service_->SetThreadPriority(PRIORITY_HIGH);
  }
  if (NULL == async_listener_.get()) {
    async_listener_ = event_service_->CreateAsyncListener();
    ASSERT_RETURN_FAILURE((NULL == async_listener_.get()), false);
    async_listener_->SignalNewConnected.connect(this, &DpServer::OnListenerAcceptEvent);
    res = async_listener_->Start(addr, true);
    if (res) {
      DLOG_INFO(MOD_EB, "Start DpServer successed(%s)", addr.ToString().c_str());
    } else {
      async_listener_->Close();
      async_listener_.reset();
      DLOG_ERROR(MOD_EB, "Start DpServer failed(%s)", addr.ToString().c_str());
    }
  }
  return res;
}

void DpServer::OnListenerAcceptEvent(AsyncListener::Ptr listener, Socket::Ptr s, int err) {
  if (nullptr == s || nullptr == listener) {
    DLOG_ERROR(MOD_EB, "param is null");
    return;
  }

  DLOG_KEY(MOD_EB, "DpServer accept remote client:%s", s->GetRemoteAddress().ToString().c_str());
  DpAgent::Ptr dp_agent = DpAgent::Ptr(new DpAgent(event_service_));
  if (NULL == dp_agent.get()) {
    DLOG_ERROR(MOD_EB, "Create Dp Client agent failed");
    return;
  }

  const bool res = dp_agent->Start(s);
  if (!res) {
    return;
  }
  dp_agent->SignalErrorEvent.connect(this, &DpServer::OnAgentErrorEvent);
  dp_agents_.emplace_back(dp_agent);
}

void DpServer::OnAgentErrorEvent(DpAgent::Ptr dp_agent, int err) {
  DLOG_ERROR(MOD_EB, "Dp Agent error:%d", err);
  const DP_CLI_LIST::iterator pos = std::find(dp_agents_.begin(), dp_agents_.end(), dp_agent);
  if (pos != dp_agents_.end()) {
    dp_agents_.erase(pos);
  } else {
    DLOG_ERROR(MOD_EB, "Not find the Dp agents");
  }
}
}  // namespace chml

