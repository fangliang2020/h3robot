#include "vty_server.h"

namespace vty {

bool VtyServer::Start(const chml::SocketAddress &address) {
  bool res = true;
  if (!server_es_.get()) {
    server_es_ = chml::EventService::CreateEventService(NULL, "vty_server");
    ASSERT_RETURN_FAILURE(!server_es_.get(), false);
  }
  if (!cmds_.get()) {
    cmds_ = VtyCmdList<VtyCmdHandler>::Ptr(new VtyCmdList<VtyCmdHandler>());
    ASSERT_RETURN_FAILURE(!cmds_.get(), false);
  }

  if (NULL == async_listener_.get()) {
    async_listener_ = server_es_->CreateAsyncListener();
    ASSERT_RETURN_FAILURE(!async_listener_.get(), false);
    async_listener_->SignalNewConnected.connect(
      this, &VtyServer::OnListenerAcceptEvent);
    res = async_listener_->Start(address, true);
    if (res) {
      address_ = address;
      DLOG_KEY(MOD_EB, "Start VtyServer succeed(%s)",
               address.ToString().c_str());
    } else {
      async_listener_->Close();
      async_listener_.reset();
      DLOG_ERROR(MOD_EB, "Start VtyServer failed(%s)",
                 address.ToString().c_str());
    }
  }
  return res;
}

bool VtyServer::Reset(const chml::SocketAddress &address) {
  DLOG_KEY(MOD_EB, "Reset VtyServer. addr: %s.", address.ToString().c_str());
  static const chml::SocketAddress temp_addr;
  if (address_ != temp_addr && address == address_) {
    DLOG_KEY(MOD_EB, "address is same as current address_, Reset failed");
    return false;
  }

  if (async_listener_) {
    async_listener_->Close();
    async_listener_.reset();
  }

  return Start(address);
};

void VtyServer::Close() {
  close_flag_ = true;
}

bool VtyServer::IsClose() {
  return close_flag_;
}


int VtyServer::RegistCmd(const char *cmd, VtyCmdHandler handler, const void *user_data, const char *info) {
  {
    //支持未InitServer时注册命令
    chml::CritScope crit(&crit_);
    if (!cmds_.get()) {
      cmds_ = VtyCmdList<VtyCmdHandler>::Ptr(new VtyCmdList<VtyCmdHandler>());
      ASSERT_RETURN_FAILURE(!cmds_.get(), -1);
    }
  }
  ASSERT_RETURN_FAILURE(cmd == NULL, -1);
  ASSERT_RETURN_FAILURE(handler == NULL, -1);

  int ret = cmds_->AddCmd(cmd, handler, user_data, info);
  if (ret != 0) {
    DLOG_WARNING(MOD_EB, "regist cmd failed. ret:%d.", ret);
  }

  return ret;
}

int VtyServer::DeregistCmd(const char *cmd) {
  {
    //支持未InitServer时取消注册命令
    chml::CritScope crit(&crit_);
    if (!cmds_.get()) {
      cmds_ = VtyCmdList<VtyCmdHandler>::Ptr(new VtyCmdList<VtyCmdHandler>());
      ASSERT_RETURN_FAILURE(!cmds_.get(), -1);
    }
  }
  ASSERT_RETURN_FAILURE(cmd == NULL, -1);

  int ret = cmds_->RemoveCmd(cmd);
  if (ret != 0) {
    DLOG_WARNING(MOD_EB, "deregist cmd failed. ret:%d.", ret);
  }
  return ret;
}

void VtyServer::OnListenerAcceptEvent(chml::AsyncListener::Ptr listener,chml::Socket::Ptr s, int err) {
  DLOG_KEY(MOD_EB, "VtyServer accept remote client: %s", s->GetRemoteAddress().ToString().c_str());
  chml::AsyncSocket::Ptr async_socket = server_es_->CreateAsyncSocket(s);
  ASSERT_RETURN_VOID(!async_socket.get());
  ASSERT_RETURN_VOID(IsClose());
  //to do reject multi connect request.

  //processor
  if (proc_vtys_.size() < VTY_MAX_CONNECTED) {
    VtyProcessor::Ptr proc_vty =
      VtyProcessor::Ptr(new VtyProcessor(server_es_, async_socket, cmds_));
    ASSERT_RETURN_VOID(!proc_vty.get());
    proc_vty->SignalErrorEvent.connect(this,
                                       &VtyServer::OnSocketErrorEvent);
    proc_vty->SignalConnectClose.connect(this,
                                         &VtyServer::OnVtyDisconnectEvent);
    proc_vtys_.push_back(proc_vty);
    int vty_start_ret = proc_vty->Start();
    ASSERT_RETURN_VOID(vty_start_ret != 0);
  } else {
    DLOG_WARNING(MOD_EB, "to many connect request. undo anything.");
  }
  return;
}

void VtyServer::OnSocketErrorEvent(VtyProcessor::Ptr proc, int err) {
  DLOG_ERROR(MOD_EB, "VtyProcessor error:%d.", err);
}

void VtyServer::OnVtyDisconnectEvent(VtyProcessor::Ptr proc, int err) {
  DLOG_WARNING(MOD_EB, "VtyProcessor disconnected:%d.", err);
  std::list<VtyProcessor::Ptr>::iterator it = proc_vtys_.begin();
  for (; it != proc_vtys_.end(); it++) {
    if (*it == proc) {
      proc_vtys_.erase(it);
      break;
    }
  }
}

}  //namespace vty
