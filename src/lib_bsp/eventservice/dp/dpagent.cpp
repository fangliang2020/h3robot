#include "eventservice/dp/dpclient.h"
#include "eventservice/dp/dpagent.h"
#include "eventservice/net/networktinterfaceimpl.h"
#ifdef ENABLE_VTY
#include "eventservice/vty/vty_client.h"
#endif

namespace chml {

#ifdef ENABLE_VTY
static void VtyRegistCmd(void);
#endif

class DpAgentManager {
 public:
  DpAgentManager() {
    agent_num_ = 0;
#ifdef ENABLE_VTY
    VtyRegistCmd();
#endif
  }
  virtual ~DpAgentManager() {
  }

  static DpAgentManager *GetInstance();

  /**
   * @brief 添加DpClient客户端，并为客户端分配唯一的ID标识
   * @param dp_agent DpClient连接处理对象
   * @param[out] id 保存分配的唯一标识
   * @return 操作是否成功。0成功，其它表示失败
   */
  int32 AddDpAgent(DpAgent::Ptr dp_agent, uint32 &id) {
    DpAgents::iterator iter = dp_agents_.begin();
    for ( ; iter != dp_agents_.end(); iter++) {
      if (dp_agent == iter->second) {
        DLOG_ERROR(MOD_EB, "Dpagent allready exist, num:%d", iter->first);
        return -1;
      }
    }

    agent_num_++;
    dp_agents_[agent_num_] = dp_agent;

    id = agent_num_;

    DLOG_INFO(MOD_EB, "Add new dp agent, id:%d", agent_num_);
    return 0;
  }

  /**
   * @brief 根据客户端标识，移除指定的客户端
   * @param id
   */
  void RemoveAgent(uint32 id) {
    if (id <= 0) {
      DLOG_ERROR(MOD_EB, "Remove dpagent failed, invalid id:%", id);
      return;
    }

    DpAgents::iterator iter = dp_agents_.find(id);
    if (iter != dp_agents_.end()) {
      for (const auto& p : method_agents_) {
        method_agents_[p.first].remove_if([&](const DpAgent::Ptr &agent) {
          return (agent->GetId() == id);
        });
      }
      dp_agents_.erase(id);
      DLOG_INFO(MOD_EB, "Remove dp agent, total:%d", dp_agents_.size());
    } else {
      DLOG_ERROR(MOD_EB, "Remove dpagent failed, not find id:%", id);
    }
  }

  DpAgent::Ptr GetAgent(uint32 id) {
    DpAgents::iterator iter = dp_agents_.find(id);
    if (iter != dp_agents_.end()) {
      return iter->second;
    } else {
      return DpAgent::Ptr();
    }
  }

  bool CheckMessage(DpAgent::Ptr dp_agent, const std::string method, uint32 type) {
    DpAgents::iterator iter = dp_agents_.begin();
    for ( ; iter != dp_agents_.end(); iter++) {
      DpAgent::Ptr dst_agent = iter->second;
      if (!(dst_agent->CheckMessage(method, type))) {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief 处理DP消息，即根据消息类型，将消息中转给订阅了相应消息类型的DpClient。
   * @param dp_agent 消息的发送者
   * @param dp_msg
   * @param[out] type 消息类别，即是同步还是广播
   * @return 处理此消息的客户端数量。若参数错误时，返回的是0
   */
  uint32 ProcessDpMessage(DpAgent::Ptr dp_agent, DpNetMsgHeader *dp_msg, uint32 &type) {
    uint32 num  = 0;
    uint32 reg_type = TYPE_INVALID;
    if (!dp_agent.get() || !dp_msg) {
      DLOG_ERROR(MOD_EB, "message is null.");
      return num;
    }

#if 1
    std::string method(dp_msg->body, dp_msg->method_size);
    // 根据消息号，获取与之相关的客户端，即获取订阅了此消息的客户端
    auto dp_agents = method_agents_.find(method);
    if (dp_agents != method_agents_.end()) {
        // 遍历每一个相关的客户端
      for (auto agent : dp_agents->second) {
        // 同一个DP
        if (agent == dp_agent) {
          continue;
        }
        // 同一个进程
        if (agent->GetPid() == dp_msg->process_id) {
          DLOG_DEBUG(MOD_EB, "the same process, pid: %d", dp_msg->process_id);
          continue;
        }
        uint32 res = agent->HandleDpMessage(dp_agent, dp_msg);
        if (TYPE_MESSAGE == res) {
          reg_type = TYPE_MESSAGE;
          num++;
        } else if (TYPE_REQUEST == res) {
            // 只有一个处理了此函数，直接退出
          reg_type = TYPE_REQUEST;
          num = 1;
          break;
        }
      }
    }
#else
    DpAgents::iterator iter = dp_agents_.begin();
    for ( ; iter != dp_agents_.end(); iter++) {
      DpAgent::Ptr dst_agent = iter->second;
      // 同一个DP
      if (dst_agent == dp_agent) {
        continue;
      }
      // 同一个进程
      if (dst_agent->GetPid() == dp_msg->process_id) {
        DLOG_DEBUG(MOD_EB, "the same process, pid: %d", dp_msg->process_id);
        continue;
      }

      std::string method(dp_msg->body, dp_msg->method_size);
      uint32 res = dst_agent->HandleDpMessage(dp_agent, dp_msg);
      if (TYPE_MESSAGE == res) {
        reg_type = TYPE_MESSAGE;
        num++;
      } else if (TYPE_REQUEST == res) {
        reg_type = TYPE_REQUEST;
        num = 1;
        break;
      }
    }
#endif
    type = reg_type;
    return num;
  }

  void DumpRegMessage(const void *vty_data) {
    DpAgents::iterator iter = dp_agents_.begin();
    for ( ; iter != dp_agents_.end(); iter++) {
      DpAgent::Ptr dp_agent = iter->second;
      if(!dp_agent.get()) {
        continue;
      }

#ifdef ENABLE_VTY
      dp_agent->DumpRegMessage(vty_data);
#endif
    }
  }

  bool AddAgentBindMethod(const std::string &method, uint32_t agent_id) {
    if (dp_agents_.count(agent_id)) {
        // 获取与消息消息相关的客户端列表
      const auto &a = method_agents_[method];
      auto iter = std::find_if(a.begin(), a.end(), [&] (const DpAgent::Ptr &agent) {
        return (agent->GetId() == agent_id);
      });
      // 同一个客户端不能多次注册同一个消息method
      if (iter != a.end()) {
        DLOG_WARNING(MOD_EB, "mutil add agent, method: %s, agent: %p", method.c_str(), dp_agents_[agent_id].get()) ;
      } else {
        method_agents_[method].emplace_back(dp_agents_[agent_id]);
      }
    } else {
      DLOG_ERROR(MOD_EB, "agent not found, id: %d", agent_id);
      return false;
    }
    return true;
  }

  bool RemoveAgentBindMethod(const std::string &method, uint32_t agent_id) {
    if (dp_agents_.count(agent_id)) {
      method_agents_[method].remove_if([&] (const DpAgent::Ptr &agent) {
        return (agent->GetId() == agent_id);
      });
    } else {
      DLOG_ERROR(MOD_EB, "agent not found, id: %d", agent_id);
      return false;
    }
    return true;
  }

 private:
  typedef std::map<uint32, DpAgent::Ptr> DpAgents;
  typedef std::map<std::string, std::list<DpAgent::Ptr>> MethodAgentsMap;
  DpAgents                dp_agents_;  // DpClient标识到处理对端之间的映射
  MethodAgentsMap         method_agents_; // 消息类型到处理对象的映射

 public:
  static DpAgentManager  *instance_;
  uint32                  agent_num_;
};

DpAgentManager *DpAgentManager::instance_ = NULL;
DpAgentManager *DpAgentManager::GetInstance() {
  if (DpAgentManager::instance_ == NULL) {
    DpAgentManager::instance_ = new DpAgentManager();
  }
  return DpAgentManager::instance_;
}

DpAgent::DpAgent(EventService::Ptr es)
  : event_service_(es)
  , remote_pid_(-1)
  , agent_name_("unnamed")
  , agent_id_(0) {
  DLOG_INFO(MOD_EB, "DpAgent construct");
  ASSERT_RETURN_VOID(!es);
  agent_manager_ = DpAgentManager::GetInstance();
  if (NULL == agent_manager_) {
    DLOG_ERROR(MOD_EB, "Get DpAgentManager instance failed");
  }
}

DpAgent::~DpAgent() {
  DLOG_INFO(MOD_EB, "DpAgent destruct");
}

bool DpAgent::Start(Socket::Ptr socket) {
  if (NULL == socket.get() 
    || NULL == agent_manager_
    || NULL == event_service_.get()) {
    DLOG_ERROR(MOD_EB, "param is null");
    return false;
  }

  AsyncSocket::Ptr async_socket = event_service_->CreateAsyncSocket(socket);
  if (NULL == async_socket.get()) {
    DLOG_ERROR(MOD_EB, "Create async socket failed");
    return false;
  }
  async_socket->SetOption(OPT_NODELAY, 1);
  AsyncPacketSocket::Ptr packet_socket(new AsyncPacketSocket(event_service_, async_socket));
  if (NULL == packet_socket.get()) {
    DLOG_ERROR(MOD_EB, "Create async packet socket failed");
    return false;
  }
  packet_socket->SignalPacketWrite.connect(this, &DpAgent::OnSocketWriteComplete);
  packet_socket->SignalPacketEvent.connect(this, &DpAgent::OnSocketReadComplete);
  packet_socket->SignalPacketError.connect(this, &DpAgent::OnSocketErrorEvent);
  packet_socket->AsyncRead();

  packet_socket_ = packet_socket;
  // 获取客户端的唯一标识
  agent_manager_->AddDpAgent(shared_from_this(), agent_id_);
  DLOG_INFO(MOD_EB, "Dp client agent init successed");
  return true;
}

void DpAgent::close() {
  if (packet_socket_.get()) {
    packet_socket_->Close();
    packet_socket_.reset();
  }

  if (agent_manager_) {
    // agent_manager_->Ragent_manager_emoveAgent(agent_id_);
    agent_manager_->RemoveAgent(agent_id_);
  }
  DLOG_INFO(MOD_EB, "DpAgent closed");
}

uint32 DpAgent::GetId() {
  return agent_id_;
}

uint32 DpAgent::GetPid() {
  return remote_pid_;
}

uint32 DpAgent::HandleDpMessage(DpAgent::Ptr src_agent, DpNetMsgHeader *dp_msg) {
  if (!src_agent.get() || !dp_msg) {
    DLOG_ERROR(MOD_EB, "param is null.");
    return TYPE_INVALID;
  }
  
  std::string method(dp_msg->body, dp_msg->method_size);
  const DP_MAP::iterator &iter = method_map_.find(method);
  if (iter != method_map_.end()) {
    uint8 type = iter->second;
    // 获取数据大小以及数据地址
    uint32 data_size = NetworkToHost32(dp_msg->data_size);
    char *data = dp_msg->body + dp_msg->method_size;
    if (TYPE_REQUEST == type) {
      // For request message, DP Agent attaches Agent-ID to Message-ID before
      // forwarding, and takes out Agent-ID after receiving replay,
      // Ensure consistency of message ID on message sender.
      // high 16 bits: agent id; low 16 bits: message id, user defined.
      uint16 src_agent_id = src_agent->GetId();
      uint16 msg_id = dp_msg->message_id & 0x0000ffff;
      dp_msg->message_id = ((src_agent_id & 0x0000ffff) << 16) | (msg_id & 0x0000ffff);
      DLOG_DEBUG(MOD_EB, "agent_id:%d, msg_id:%d", src_agent_id, msg_id);
    }

    bool res = SendMessage(dp_msg->message_id, type, method,
                           data, data_size, dp_msg->session_id, 0);
    if (!res) {
      DLOG_ERROR(MOD_EB, "Transmit Dp message failed");
    }

    dp_msg->type = type;
    return type;
  }
  return TYPE_INVALID;
}

bool DpAgent::CheckMessage(const std::string method, uint32 type) {
  DP_MAP::iterator iter = method_map_.find(method);
  if (iter != method_map_.end()) {
    uint32 reg_type = iter->second;
    if (reg_type != type) {
      DLOG_ERROR(MOD_EB, "Method \"%s\" type %d conflict",
                 method.c_str(), type);
      return false;
    } else if (TYPE_REQUEST == type) {
      DLOG_ERROR(MOD_EB, "Repeated DP request method:%s",
                 method.c_str());
      return false;
    }
  }
  return true;
}

void DpAgent::OnSocketWriteComplete(
  AsyncPacketSocket::Ptr async_socket) {
  //DLOG_INFO(MOD_EB, "Socket send data done");
}

void DpAgent::OnSocketReadComplete(AsyncPacketSocket::Ptr async_socket, MemBuffer::Ptr data, uint16 flag) {
  std::string packet = data->ToString();
  if (packet.size() <= sizeof(DpNetMsgHeader)) {
    DLOG_ERROR(MOD_EB, "Invalid Dp message length:%d", packet.size());
    return;
  }
  
  DpNetMsgHeader *dpmsg = (DpNetMsgHeader*)packet.c_str();
  // DLOG_DEBUG(MOD_EB, "on read type:%d message id:%d.\n", dpmsg->type, dpmsg->message_id);
  switch (dpmsg->type) {
  case TYPE_MESSAGE: {
    (void)OnRequestMessage(dpmsg);
    break;
  }
  case TYPE_REPLY: {
    (void)OnReplayMessage(dpmsg);
    break;
  }
  case TYPE_ADD_MESSAGE: {
    (void)OnListenMessage(dpmsg);
    break;
  }
  case TYPE_REMOVE_MESSAGE: {
    (void)OnRemoveMessage(dpmsg);
    break;
  }
  case TYPE_SET_NAME: {
    (void)OnSetClientName(dpmsg);
    break;
  }
  default: {
    DLOG_INFO(MOD_EB, "Invalid Dp message received, type:%d", dpmsg->type);
    break;
  }
  }
  (void)packet_socket_->AsyncRead();
}

void DpAgent::OnSocketErrorEvent(AsyncPacketSocket::Ptr async_socket,
                                 int err) {
  SignalErrorEvent(shared_from_this(), err);
  close();
}

bool DpAgent::OnListenMessage(DpNetMsgHeader *dpmsg) {
  if (!dpmsg) {
    DLOG_ERROR(MOD_EB, "param is null.");
    return false;
  }
  
  std::string method(dpmsg->body, dpmsg->method_size);

  bool res = false;
  do {
    uint8 type = dpmsg->listen_type;
    if ((method.size() == 0)
        || ((TYPE_MESSAGE != type)
            && (TYPE_REQUEST != type))) {
      DLOG_ERROR(MOD_EB, "Listen message failed, invalid method:%s, type:%d",
                 method.c_str(), type);
      break;
    }
    // 消息类别要与原有注册的类别一致，并且Request类别消息，只能有一个监听者。
    if (!(agent_manager_->CheckMessage(shared_from_this(), method, type))) {
      DLOG_ERROR(MOD_EB, "Listen message failed(method:%s, type:%d)", method.c_str(), type);
      break;
    }
    // 保存客户端监听的消息，及其类型
    method_map_[method] = dpmsg->listen_type;
    agent_manager_->AddAgentBindMethod(method, agent_id_);
    
    res = true;
    DLOG_INFO(MOD_EB, "Listen message success(method:%s, type:%d)", method.c_str(), type);
  } while (0);

  int result = res ? TYPE_SUCCEED : TYPE_FAILURE;
  SendMessage(dpmsg->message_id, TYPE_ADD_MESSAGE, method, NULL, 0, dpmsg->session_id, result);
  return true;
}

bool DpAgent::OnRemoveMessage(DpNetMsgHeader *dpmsg) {
  if (!dpmsg) {
    DLOG_ERROR(MOD_EB, "param is null.");
    return false;
  }

  std::string method(dpmsg->body, dpmsg->method_size);
  agent_manager_->RemoveAgentBindMethod(method, agent_id_);
  method_map_.erase(method);
  //SendMessage(dpmsg->message_id, TYPE_REMOVE_MESSAGE, method, NULL, 0, dpmsg->session_id, TYPE_SUCCEED);
  return true;
}

bool DpAgent::OnSetClientName(DpNetMsgHeader *dpmsg) {
  if (!dpmsg) {
    DLOG_ERROR(MOD_EB, "param is null.");
    return false;
  }
  
  if (dpmsg->method_size > 0) {
    uint16 length = (dpmsg->method_size > 32) ? 32 : dpmsg->method_size;
    
    // 保存客户端的进程号，并更新名称
    remote_pid_ = dpmsg->process_id;
    agent_name_.assign(dpmsg->body, length);
    DLOG_INFO(MOD_EB, "Config DPAgent name:%s", agent_name_.c_str());
  }
  return true;
}

// Network message(broadcast or request) handle function.
bool DpAgent::OnRequestMessage(DpNetMsgHeader *dpmsg) {
  if (!dpmsg || !agent_manager_) {
    DLOG_ERROR(MOD_EB, "param is null.");
    return false;
  }

  std::string method(dpmsg->body, dpmsg->method_size);
  uint32 type = TYPE_INVALID;
  uint32 num = agent_manager_->ProcessDpMessage(shared_from_this(), dpmsg, type);
  if ((0 == num)  || (TYPE_MESSAGE == type)) {
    if (0 == num) {
      DLOG_WARNING(MOD_EB, "nobody listening the message:%s", method.c_str());
    }
    SendMessage(dpmsg->message_id, TYPE_REPLY, method, NULL, 0, dpmsg->session_id, num);
  }
  return true;
}

// DP请求网络Replay消息处理函数
bool DpAgent::OnReplayMessage(DpNetMsgHeader *dpmsg) {
  if (!dpmsg) {
    DLOG_ERROR(MOD_EB, "param is null.");
    return false;
  }
  
  // For request message, DP Agent attaches Agent-ID to Message-ID before
  // forwarding, and takes out Agent-ID after receiving replay,
  // Ensure consistency of message ID on message sender.
  // high 16 bits: agent id; low 16 bits: message id, user defined.
  uint32 src_msgid = dpmsg->message_id & 0x0000ffff;
  uint16 src_agent_id = (dpmsg->message_id & 0xffff0000) >> 16;
  DLOG_DEBUG(MOD_EB, "agent_id:%d, msg_id:%d", src_agent_id, src_msgid);

  DpAgent::Ptr src_agent = agent_manager_->GetAgent(src_agent_id);
  if (src_agent) {
    std::string method;
    method.append(dpmsg->body, dpmsg->method_size);

    char *pdata = dpmsg->body + dpmsg->method_size;
    uint32 ndata = NetworkToHost32(dpmsg->data_size);

    src_agent->SendMessage(src_msgid, TYPE_REPLY, method, pdata, ndata, dpmsg->session_id, 1);
    return true;
  } else {
    std::string method;
    method.append(dpmsg->body, dpmsg->method_size);
    DLOG_ERROR(MOD_EB, "Invalid DP Replay msg, id %d, method:%s",
               src_msgid, method.c_str());
    return false;
  }
}

bool DpAgent::SendMessage(uint32 msg_id, uint8 type, std::string method,
                          const char *data, uint32 data_size, uint32 session_id,
                          uint32 result) {
  if (NULL == packet_socket_.get()) {
    DLOG_ERROR(MOD_EB, "Invalid Socket, Send message failed"
               "(msg_id:%d, type:%d, method:%s)",
               msg_id, type, method.c_str());
    return false;
  }

  uint32 method_len = method.size();
  chml::MemBuffer::Ptr dpbuf = chml::MemBuffer::CreateMemBuffer();
  ASSERT_RETURN_FAILURE(!dpbuf, false);

  DpNetMsgHeader dpmsg = { 0 };
  dpmsg.type = type;
  dpmsg.message_id = msg_id;
  dpmsg.method_size = method_len;
  dpmsg.session_id = session_id;
  dpmsg.result = result;
  dpmsg.data_size = HostToNetwork32(data_size);
  dpbuf->WriteBytes((const char*)&dpmsg, sizeof(DpNetMsgHeader));
  dpbuf->WriteBytes(method.c_str(), method_len);
  if (data_size > 0) {
    dpbuf->WriteBytes(data, data_size);
  }
  bool ret = packet_socket_->AsyncWritePacket(dpbuf, 0);
  if (!ret) {
    DLOG_ERROR(MOD_EB, "Send message failed(msg_id:%d, type:%d, method:%s)",
               msg_id, type, method.c_str());
  }

  return ret;
}

#ifdef ENABLE_VTY
// Prints currently registered DP messages.
void DpAgent:: DumpRegMessage(const void *vty_data) {
  Vty_Print(vty_data, ">> DP Client \"%s\", listening total %d messages:\n",
            agent_name_.c_str(), (int)method_map_.size());
  for (DP_MAP::iterator iter = method_map_.begin();
       iter != method_map_.end(); iter++) {
    Vty_Print(vty_data, "    method: %-32s, type: %s\n",
              iter->first.c_str(),
              ((0 == iter->second) ? "MESSAGE" : "REQUEST"));
  }
}

static void VtyDumpListenMsg(const void *vty_data,
                             const void *user_data,
                             int argc,
                             const char *argv[]) {
  DpAgentManager *agent_mgr = DpAgentManager::GetInstance();
  if (NULL == agent_mgr) {
    return;
  }
  agent_mgr->DumpRegMessage(vty_data);
}

static void VtyRegistCmd(void) {
  (void)Vty_RegistCmd("chml_dp_show_listen_msg",
                      (VtyCmdHandler)VtyDumpListenMsg,
                      NULL,
                      "print all registered DP message information");
}
#endif

}
