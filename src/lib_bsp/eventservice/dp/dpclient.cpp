#include <map>

#include "eventservice/dp/dpclient.h"
#include "eventservice/net/asyncpacketsocket.h"
#include "eventservice/net/networktinterfaceimpl.h"

#include "eventservice/log/log_client.h"
#include "eventservice/util/proc_util.h"

namespace chml {

#define DP_CLI_CONNECT_TIMEOUT    (3*1000) //
#define DP_CLI_REQUEST_TIMEOUT    (5*1000) //
#define DP_RECONNECT_MESSAGE_ID   2        //

class DpLocalDisp;

///DpClientImpl////////////////////////////////////////////////////////////////
class DpClientImpl : public DpClient,
  public boost::noncopyable,
  public sigslot::has_slots<>,
  public MessageHandler,
  public boost::enable_shared_from_this<DpClientImpl>  {
 public:
  typedef boost::shared_ptr<DpClientImpl> Ptr;

 public:
  DpClientImpl(EventService::Ptr es, const std::string &name)
    : msg_proc_evs_(es),
      dp_cli_name_(name),
      message_id_(0) {
    DLOG_INFO(MOD_EB, "DpClient construct");
  }

  ~DpClientImpl() {
    Close();
    DLOG_INFO(MOD_EB, "DpClient destruct");
  }


  bool Init(const SocketAddress srv_addr) {
    DLOG_INFO(MOD_EB, "DpClient init request");
    dp_srv_addr_ = srv_addr;

    // 端口为0,单进程处理
    if (0 == dp_srv_addr_.port()) {
      DLOG_ERROR(MOD_EB, "addr is null, will all in one");
      return true;
    }

    EventService::Ptr evs = DpClient::DpClientEventService();
    if (NULL == evs.get()) {
      DLOG_ERROR(MOD_EB, "create eventservice failde.");
      return false;
    }
    signal_event_ = GetSignal();
    ASSERT_RETURN_FAILURE(!signal_event_, false)

    async_connect_ = evs->CreateAsyncConnect();
    if (NULL == async_connect_.get()) {
      DLOG_ERROR(MOD_EB, "Create async connector failed");
      PutSignal(signal_event_);
      return false;
    }
    async_connect_->SignalServerConnected.connect(this, &DpClientImpl::OnSocketConnectEvent);
    bool res = async_connect_->Connect(dp_srv_addr_, 10*1000);
    if (!res) {
      DLOG_ERROR(MOD_EB, "Async connector Connect failed");
      PutSignal(signal_event_);
      return false;
    }

    int32 ret = signal_event_->WaitSignal(DP_CLI_CONNECT_TIMEOUT);
    if (ret != SIGNAL_EVENT_DONE) {
      signal_event_->ResetTriggerSignal();
      DLOG_INFO(MOD_EB, "DpClient %s init failed, err:%d", dp_cli_name_.c_str(), ret);
      PutSignal(signal_event_);
      return false;
    }

    DLOG_INFO(MOD_EB, "DpClient %s init successed", dp_cli_name_.c_str());
    PutSignal(signal_event_);
    return true;
  }

  bool ReConnect() {
    //msg_proc_
    msg_proc_evs_->Clear(this, DP_RECONNECT_MESSAGE_ID);
    msg_proc_evs_->PostDelayed(3000, this,DP_RECONNECT_MESSAGE_ID);
    return true;
  }

  bool ReListen() {
    for (auto it = method_map_.begin(); it != method_map_.end(); ++it) {
      ListenMessage(it->first, it->second);
    }
    return true;
  }

  void Close() {
    DLOG_KEY(MOD_EB, "Close DpClient");
    // 避免Socket发生异常时，Service线程和用户线程并发，
    // 对象被销毁导致异常
    chml::CritScope cr(&dp_req_crit_);
    request_map_.clear();
    if (packet_socket_) {
      packet_socket_->Close();
      packet_socket_.reset();
    }
    if (async_connect_) {
      async_connect_->Close();
      async_connect_.reset();
    }
    if (!SignalErrorEvent.is_empty()) {
      SignalErrorEvent.disconnect_all();
    }
    if (!SignalDpMessage.is_empty()) {
      SignalDpMessage.disconnect_all();
    }
  }

  virtual int32 SendDpMessage(const std::string method, uint32 session_id,
                              const char *data, uint32 data_size,
                              DpBuffer *res_buffer, uint32 timeout);

  virtual int32 SendRemoteMessage(const std::string method, uint32 type, uint32 session_id,
                              const char* data, uint32 data_size,
                              DpBuffer* res_buffer, uint32 timeout);

  virtual bool SendDpReply(DpMessage::Ptr dp_msg);
  /**
   *
   * @param[in] method:监听消息
   * @param[in] type:消息类型;TYPE_REQUEST,TYPE_MESSAGE
   * @return    >0 执行成功,文件内容长度;<=0 失败,详见return_define.h
   * @history
   */
  virtual int32 ListenMessage(const std::string method, uint32 type);
  virtual void  RemoveMessage(const std::string method);

  /**
   * 本地消息发送
   * @param[in]  dp_client:消息发出dp客户端
   * @param[in]  dp_msg:dp消息
   * @return     成功:返回消息类型TYPE_REQUEST;失败:TYPE_INVALID
   * @history
   */
  int32 LocalHandleRequest(DpClientImpl::Ptr dp_client, DpMessage::Ptr dp_msg);

 private:
  void OnSocketConnectEvent(chml::AsyncConnecter::Ptr async_client, chml::Socket::Ptr s, int32 err) {
    if (err || !s) {
      DLOG_ERROR(MOD_EB, "Connect remote failed");
      return;
    }
    DLOG_INFO(MOD_EB, "Connect remote %s successed", s->GetRemoteAddress().ToString().c_str());

    EventService::Ptr service_es = DpClient::DpClientEventService();
    ASSERT_RETURN_VOID(!service_es);

    AsyncSocket::Ptr async_socket = service_es->CreateAsyncSocket(s);
    ASSERT_RETURN_VOID(!async_socket);

    AsyncPacketSocket::Ptr ap_socket(new AsyncPacketSocket(service_es, async_socket));
    ASSERT_RETURN_VOID(!ap_socket);

    ap_socket->SignalPacketWrite.connect(this, &DpClientImpl::OnSocketWriteComplete);
    ap_socket->SignalPacketEvent.connect(this, &DpClientImpl::OnSocketReadComplete);
    ap_socket->SignalPacketError.connect(this, &DpClientImpl::OnSocketErrorEvent);

    ap_socket->AsyncRead();
    packet_socket_ = ap_socket;

    // 配置client端名称
    uint32 message_id = ++message_id_;
    bool res = SendDpMessage(message_id, TYPE_SET_NAME, TYPE_INVALID, dp_cli_name_, NULL, 0, 0);
    if (!res) {
      DLOG_WARNING(MOD_EB, "set client name faild", dp_cli_name_.c_str());
    }
    // 唤醒用户线程
    signal_event_->TriggerSignal();
  }

  void OnSocketWriteComplete(AsyncPacketSocket::Ptr async_socket) {
    //DLOG_INFO(MOD_EB, "Socket send data done");
  }

  void OnSocketReadComplete(AsyncPacketSocket::Ptr async_socket, MemBuffer::Ptr data, uint16 flag) {
    std::string packet = data->ToString();
    if (packet.size() <= sizeof(DpNetMsgHeader)) {
      DLOG_ERROR(MOD_EB, "Invalid Dp message length:%d", packet.size());
      return;
    }

    DpNetMsgHeader *dpmsg = (DpNetMsgHeader*)packet.c_str();
    switch (dpmsg->type) {
    case TYPE_MESSAGE: {
      (void)OnBroadCastMessage(dpmsg);
      break;
    }
    case TYPE_REQUEST: {
      (void)OnRequestMessage(dpmsg);
      break;
    }
    case TYPE_REPLY: {
      (void)OnReqReplayMessage(dpmsg);
      break;
    }
    case TYPE_ADD_MESSAGE: {
      (void)OnAddReplayMessage(dpmsg);
      break;
    }
    default: {
      DLOG_WARNING(MOD_EB, "Invalid Dp message received, type:%d", dpmsg->type);
      break;
    }
    }
    packet_socket_->AsyncRead();
  }

  void OnSocketErrorEvent(AsyncPacketSocket::Ptr async_socket, int32 err) {
    DLOG_ERROR(MOD_EB, "AsyncPacketSocket error:%d", err);
    // 避免Socket发生异常时，Service线程和用户线程并发，
    // 对象被销毁导致异常
    chml::CritScope cr(&dp_req_crit_);
    request_map_.clear();
    if (packet_socket_) {
      packet_socket_->Close();
      packet_socket_.reset();
    }
    msg_proc_evs_->Clear(this,DP_RECONNECT_MESSAGE_ID);
    msg_proc_evs_->PostDelayed(1000, this,DP_RECONNECT_MESSAGE_ID);
//    SignalErrorEvent(shared_from_this(), TYPE_ERROR_DISCONNECTED);
  }

  virtual void OnMessage(Message *msg) {
    uint32 msg_size = msg_proc_evs_->size();
    if (msg->message_id == DP_RECONNECT_MESSAGE_ID) {
      if(Init(dp_srv_addr_)) {
        ReListen();
      }
      else {
        ReConnect();
      }
      return;
    }

    if(msg_size > 10) {
      DLOG_ERROR(MOD_EB, "%s hold dp message %u.", dp_cli_name_.c_str(), msg_size);
    }

    if (packet_socket_) {
      uint32 recv_block = packet_socket_->RecvBlock();
      if(recv_block > 10) {
        DLOG_ERROR(MOD_EB, "%s dp recv block %u.", dp_cli_name_.c_str(), recv_block);
      }

      uint32 send_block = packet_socket_->SendBlock();
      if(send_block > 10) {
        DLOG_ERROR(MOD_EB, "%s dp send block %u.", dp_cli_name_.c_str(), send_block);
      }
    }

    uint64 dp_end, dp_bng = chml::XTimeUtil::ClockMSecond();

    DpMessage::Ptr dp_msg =
      boost::dynamic_pointer_cast<DpMessage>(msg->pdata);
    SignalDpMessage(shared_from_this(), dp_msg);

    dp_end = chml::XTimeUtil::ClockMSecond();
    if ((dp_end - dp_bng) > 100) {
      DLOG_ERROR(MOD_EB, "%s dp process time: %lldms. method %s", dp_cli_name_.c_str(), (dp_end - dp_bng), dp_msg->method.c_str());
    }
  }

 private:
  bool OnBroadCastMessage(DpNetMsgHeader *dpmsg) {
    DpMessage::Ptr dp_msg(new DpMessage);
    dp_msg->method.append(dpmsg->body, dpmsg->method_size);
    dp_msg->message_id = dpmsg->message_id;
    dp_msg->session_id = dpmsg->session_id;
    dp_msg->type = TYPE_MESSAGE;
    char *data = dpmsg->body + dpmsg->method_size;
    uint32 data_size = NetworkToHost32(dpmsg->data_size);
    dp_msg->req_buffer.append(data, data_size);
    dp_msg->tmp_res = "";
    dp_msg->res_buffer = &dp_msg->tmp_res;
    dp_msg->signal_event = NULL;

    msg_proc_evs_->Post(this, 0, dp_msg);
    return true;
  }

  bool OnRequestMessage(DpNetMsgHeader *dpmsg) {
    DpMessage::Ptr dp_msg(new DpMessage);
    ASSERT_RETURN_FAILURE(!dp_msg, false);

    dp_msg->method.append(dpmsg->body, dpmsg->method_size);
    dp_msg->message_id = dpmsg->message_id;
    dp_msg->session_id = dpmsg->session_id;
    dp_msg->type = TYPE_REQUEST;
    char *data = dpmsg->body + dpmsg->method_size;
    uint32 data_size = NetworkToHost32(dpmsg->data_size);
    dp_msg->req_buffer.append(data, data_size);
    dp_msg->tmp_res = "";
    dp_msg->res_buffer = &dp_msg->tmp_res;
    dp_msg->is_timeout = false;
    dp_msg->signal_event = NULL;

    msg_proc_evs_->Post(this, 0, dp_msg);
    return true;
  }

  // Dp Message(request, braodcast) replay handle function
  bool OnReqReplayMessage(DpNetMsgHeader *dpmsg) {
    chml::CritScope cs(&dp_req_crit_);
    DP_REQ_MAP::iterator iter = request_map_.find(dpmsg->message_id);
    if (iter != request_map_.end()) {
      DpMessage::Ptr dp_msg = iter->second;
      //if (dp_msg->type != TYPE_MESSAGE) {
      //  DLOG_ERROR(MOD_EB, "Invalid DP Replay msg type %d", dp_msg->type);
      //  return false;
      //}
      if (dp_msg->is_timeout) {
        DLOG_ERROR(MOD_EB, "");
        std::string method;
        method.append(dpmsg->body, dpmsg->method_size);

        uint32 data_size = NetworkToHost32(dpmsg->data_size);
        char *data = dpmsg->body + dpmsg->method_size;
        std::string resp;
        resp.append(data, data_size);

        DLOG_ERROR(MOD_EB, "dp %p reply %s[%d] timeout %s", this, method.c_str(), dpmsg->message_id, resp.c_str());
        return false;
      } else {
        uint32 data_size = NetworkToHost32(dpmsg->data_size);
        if (data_size > 0) {
          char *data = dpmsg->body + dpmsg->method_size;
          dp_msg->res_buffer->append(data, data_size);
        }
        dp_msg->type = dpmsg->type;
        dp_msg->result = dpmsg->result;
        dp_msg->signal_event->TriggerSignal();

        uint32 ndiff = (uint32)chml::XTimeUtil::ClockMSecond() - dp_msg->session_id;
        if (ndiff > 1000) {
          DLOG_ERROR(MOD_EB, "dp %p reply %s[%d] will timeout %d",
            this, iter->second->method.c_str(), iter->second->message_id, ndiff);
        }
        return true;
      }
    } else {
      std::string method;
      method.append(dpmsg->body, dpmsg->method_size);

      uint32 data_size = NetworkToHost32(dpmsg->data_size);
      char *data = dpmsg->body + dpmsg->method_size;
      std::string resp;
      resp.append(data, data_size);

      DLOG_ERROR(MOD_EB, "dp %p reply %s[%d] timeout %s", this, method.c_str(), dpmsg->message_id, resp.c_str());
      return false;
    }
  }

  // "Add listen message" replay handle function
  bool OnAddReplayMessage(DpNetMsgHeader *dpmsg) {
    chml::CritScope cs(&dp_req_crit_);

    DP_REQ_MAP::iterator iter = request_map_.find(dpmsg->message_id);
    if (iter != request_map_.end()) {
      DpMessage::Ptr dp_msg = iter->second;
      if (dp_msg->type != TYPE_ADD_MESSAGE) {
        DLOG_ERROR(MOD_EB, "Invalid replay msg type", dp_msg->type);
        return false;
      }
      if (dp_msg->is_timeout) {
        return false;
      } else {
        dp_msg->result = dpmsg->result;
        dp_msg->signal_event->TriggerSignal();
        return true;
      }
    } else {
      return false;
    }
  }

  bool SendDpMessage(uint32 msg_id, uint8 type, uint8 listen_type,
                     std::string method, const char *data, uint32 data_size,
                     uint32 session_id) {
    if(0xfffe < message_id_) {  // 消息ID有效位为16bit
      message_id_ = 0;
    }

    uint32 method_len = method.size();
    chml::MemBuffer::Ptr dpbuf = chml::MemBuffer::CreateMemBuffer();
    ASSERT_RETURN_FAILURE(!dpbuf, false);

    DpNetMsgHeader dpmsg = { 0 };
    dpmsg.type = type;
    dpmsg.listen_type = listen_type;
    dpmsg.method_size = method_len;
    dpmsg.process_id = chml::XProcUtil::self_pid();
    dpmsg.message_id = msg_id;
    dpmsg.session_id = session_id;
    dpmsg.result = 0;
    dpmsg.data_size = HostToNetwork32(data_size);
    dpbuf->WriteBytes((const char*)&dpmsg, sizeof(DpNetMsgHeader));
    dpbuf->WriteBytes(method.c_str(), method_len);
    if (data_size > 0) {
      dpbuf->WriteBytes(data, data_size);
    }
    bool ret = packet_socket_ ? packet_socket_->AsyncWritePacket(dpbuf, 0) : false;
    if (!ret) {
      DLOG_ERROR(MOD_EB, "Send message failed(socket:%p, msg_id:%d, type:%d, method:%s)",
                 packet_socket_.get(), msg_id, type, method.c_str());
    }
    return ret;
  }

  SignalEvent::Ptr GetSignal() {
    CritScope cs(&signal_mutex_);
    if (signal_queue_.size() > 0) {
      SignalEvent::Ptr signal = signal_queue_.front();
      signal_queue_.pop();
      return signal;
    }
    return chml::SignalEvent::CreateSignalEvent();
  }

  void PutSignal(SignalEvent::Ptr signal) {
    CritScope cs(&signal_mutex_);
    if (signal) {
      signal_queue_.push(signal);
    }
  }

  size_t CountSignal() {
    CritScope cs(&signal_mutex_);
    return signal_queue_.size();
  }

 private:
  std::string               dp_cli_name_;
  SocketAddress             dp_srv_addr_;    // server address
  AsyncConnecter::Ptr       async_connect_;  // Tcp Connector
  AsyncPacketSocket::Ptr    packet_socket_;  // Tcp Socket

 private:
  typedef std::map<uint32, DpMessage::Ptr> DP_REQ_MAP;
  CriticalSection           dp_req_crit_;
  DP_REQ_MAP                request_map_;    // DP Request消息缓存
  uint16                    message_id_;     // DP Message、Request消息计数,自增
  uint16                    reserved;        // 备用

 private:
  typedef std::map<std::string, uint32> MSG_MAP;
  MSG_MAP                   method_map_;     // message
  CriticalSection           method_crit_;
  EventService::Ptr         msg_proc_evs_;   // 用户工作线程

 private:
  CriticalSection              signal_mutex_;   //
  SignalEvent::Ptr             signal_event_;   // connect signal
  std::queue<SignalEvent::Ptr> signal_queue_;   // message signal
};

///DpLocalDisp 本地消息分发////////////////////////////////////////////////////
class DpLocalDisp {
 private:
  DpLocalDisp() {
  }

 public:
  virtual ~DpLocalDisp() {
  }

  static DpLocalDisp *Instance() {
    static DpLocalDisp *pthiz = NULL;
    if (NULL == pthiz) {
      pthiz = new DpLocalDisp();
    }
    return pthiz;
  }

  /**
   * 注册消息处理dp客户端
   * @param[in] dp_client:消息处理dp客户端
   * @return
   * @history
   */
  void AddDpClient(DpClientImpl::Ptr dp_client) {
    chml::CritScope cs(&dp_mutex_);

    DP_CLI_LIST::iterator pos;
    pos = std::find(dp_clis_.begin(), dp_clis_.end(), dp_client);
    if (pos != dp_clis_.end()) {
      DLOG_ERROR(MOD_EB, "Can't add dp client");
      return;
    }
    dp_clis_.push_back(dp_client);
  }

  /**
  * 取消注册消息处理dp客户端
  * @param[in] dp_client:消息处理dp客户端
  * @return
  * @history
  */
  void RemoveDpClient(DpClientImpl::Ptr dp_client) {
    chml::CritScope cs(&dp_mutex_);

    DP_CLI_LIST::iterator pos;
    pos = std::find(dp_clis_.begin(), dp_clis_.end(), dp_client);
    if (pos == dp_clis_.end()) {
      DLOG_ERROR(MOD_EB, "the dpclient is not find");
      return;
    }
    dp_clis_.erase(pos);
  }

  /**
   * 处理request\message消息
   * @param[in] dp_client:消息来源dp
   * @param[in] dp_msg:消息体
   * @return
   * @history
   */
  int32 ProcessDpRequest(DpClientImpl::Ptr dp_client, DpMessage::Ptr dp_msg, uint32 &etype) {
    chml::CritScope cs(&dp_mutex_);

    etype = TYPE_INVALID;
    int32 msg_proc_time = 0;     // 消息出来次数
    DP_CLI_LIST::iterator iter = dp_clis_.begin();
    for ( ; iter != dp_clis_.end(); iter++) {
      if ((*iter) == dp_client) {
        continue;
      }

      int32 nt = (*iter)->LocalHandleRequest(dp_client, dp_msg);
      if (TYPE_INVALID != nt) {
        etype = nt;
        msg_proc_time++;
        if (TYPE_REQUEST == nt) {
          break;
        }
      }
    }
    return msg_proc_time;
  }

  /**
   * 处理reply消息
   * @param[in] dp_client:消息来源dp
   * @param[in] dp_msg:消息体
   * @return
   * @history
   */
  bool ProcessDpReply(DpClientImpl::Ptr dp_client, DpMessage::Ptr dp_msg) {
    if (dp_msg.get() && dp_msg->signal_event) {
      DLOG_DEBUG(MOD_EB, "message id %u do reply", dp_msg->message_id);
      dp_msg->signal_event->TriggerSignal();
      return true;
    }
    return false;
  }

 private:
  typedef std::list<DpClientImpl::Ptr> DP_CLI_LIST;
  DP_CLI_LIST            dp_clis_;
  chml::CriticalSection  dp_mutex_;
};

//////////////////////////////////////////////////////////////////////////
int32 DpClientImpl::SendDpMessage(const std::string method, uint32 session_id,
                                  const char *pdata, uint32 ndata,
                                  DpBuffer *res_buffer, uint32 timeout) {
  if (0 == method.size()) {
    DLOG_ERROR(MOD_EB, "Send Dp message failed, method len:0");
    return -1;
  }

  // 避免Socket发生异常时，Service线程和用户线程并发，
  // 对象被销毁导致异常
  //chml::CritScope cr(&crit_);
  DpMessage::Ptr dp_msg(new DpMessage);
  ASSERT_RETURN_FAILURE(!dp_msg, -1);

  int32 ret = TYPE_FAILURE;
  BEGIN_TIME_RECORD(__start_time__);
  do {
    dp_msg->signal_event = GetSignal();
    ASSERT_RETURN_FAILURE(!dp_msg->signal_event, ret)

    dp_msg->method = method;
    dp_msg->message_id = ++message_id_;  //  ML_AAF(&message_id_, 1);
    dp_msg->session_id = (uint32)chml::XTimeUtil::ClockMSecond();
    dp_msg->type = TYPE_MESSAGE;
    dp_msg->req_buffer.assign(pdata, ndata);
    dp_msg->tmp_res = "";
    dp_msg->res_buffer = &dp_msg->tmp_res;
    dp_msg->is_timeout = false;
    dp_msg->result = TYPE_SUCCEED;
    {
      chml::CritScope cs(&dp_req_crit_);
      request_map_[dp_msg->message_id] = dp_msg;
    }

    // 本地转发
    uint32 etype = TYPE_INVALID;
    if (DpLocalDisp::Instance()) {
      DLOG_DEBUG(MOD_EB, "dp %p message %s[%d] %d", this, method.c_str(), dp_msg->message_id, timeout);
      int32 proc_time = DpLocalDisp::Instance()->ProcessDpRequest(shared_from_this(), dp_msg, etype);
    }

    /* 本地转发后,服务端转发 */
    if (TYPE_REQUEST == etype) {
      // etype=已经处理了TYPE_REQUEST,可以不进行服务端转发
    } else if(0 == dp_srv_addr_.port()) {
      // 服务端地址为空,可以不进行服务端转发
      // 服务端地址为空并且处理的是TYPE_MESSAGE,可以不等反馈
      if (TYPE_MESSAGE == etype) {
        chml::CritScope cs(&dp_req_crit_);
        request_map_.erase(dp_msg->message_id);
        break;
      }
    } else {
      bool bret = SendDpMessage(dp_msg->message_id, TYPE_MESSAGE, TYPE_INVALID, method, pdata, ndata, session_id);
      if (!bret) {
        DLOG_ERROR(MOD_EB, "Send Dp message failed");
        chml::CritScope cs(&dp_req_crit_);
        request_map_.erase(dp_msg->message_id);
        break;
      }
    }

    // 等待Server端回复
    int32 res = dp_msg->signal_event->WaitSignal(timeout);
    if (SIGNAL_EVENT_DONE != res) { // Wait signal失败或超时，未收到回复结果
      dp_msg->is_timeout = true;
      dp_msg->signal_event->ResetTriggerSignal();
      DLOG_ERROR(MOD_EB, "dp %p message %s[%d] %d Wait DP Replay failed", this, method.c_str(), dp_msg->message_id, timeout);

      chml::CritScope cs(&dp_req_crit_);
      request_map_.erase(dp_msg->message_id);
    } else {                        // 收到回复结果
      if (res_buffer) {
        *res_buffer = *dp_msg->res_buffer;
      }

      chml::CritScope cs(&dp_req_crit_);
      request_map_.erase(dp_msg->message_id);
      ret = dp_msg->result;
    }
  } while (0);
  PutSignal(dp_msg->signal_event);
  POINT_TIME_SPAN_IF(__start_time__, 3000, 9090, "method:%s,data:%s", method.c_str(), pdata);
  return ret;
}

int32 DpClientImpl::SendRemoteMessage(const std::string method, uint32 type, uint32 session_id,
                                  const char* pdata, uint32 ndata,
                                  DpBuffer* res_buffer, uint32 timeout) {
  if (0 == method.size()) {
    DLOG_ERROR(MOD_EB, "Send Dp message failed, method len:0");
    return -1;
  }

  // 避免Socket发生异常时，Service线程和用户线程并发，
  // 对象被销毁导致异常
  //chml::CritScope cr(&crit_);
  DpMessage::Ptr dp_msg(new DpMessage);
  ASSERT_RETURN_FAILURE(!dp_msg, -1);

  int32 ret = TYPE_FAILURE;
  BEGIN_TIME_RECORD(__start_time__);
  do {
    dp_msg->signal_event = GetSignal();
    ASSERT_RETURN_FAILURE(!dp_msg->signal_event, ret)

    dp_msg->method = method;
    dp_msg->message_id = ++message_id_;  //  ML_AAF(&message_id_, 1);
    dp_msg->session_id = (uint32)chml::XTimeUtil::ClockMSecond();
    dp_msg->type = TYPE_MESSAGE;
    dp_msg->req_buffer.assign(pdata, ndata);
    dp_msg->tmp_res = "";
    dp_msg->res_buffer = &dp_msg->tmp_res;
    dp_msg->is_timeout = false;
    dp_msg->result = TYPE_SUCCEED;
    {
      chml::CritScope cs(&dp_req_crit_);
      request_map_[dp_msg->message_id] = dp_msg;
    }

    if(0 == dp_srv_addr_.port()) {
      // 服务端地址为空,可以不进行服务端转发
      // 服务端地址为空并且处理的是TYPE_MESSAGE,可以不等反馈
      if (TYPE_MESSAGE == type) {
        chml::CritScope cs(&dp_req_crit_);
        request_map_.erase(dp_msg->message_id);
        break;
      }
    } else {
      bool bret = SendDpMessage(dp_msg->message_id, TYPE_MESSAGE, TYPE_INVALID, method, pdata, ndata, session_id);
      if (!bret) {
        DLOG_ERROR(MOD_EB, "Send Dp message failed");
        chml::CritScope cs(&dp_req_crit_);
        request_map_.erase(dp_msg->message_id);
        break;
      }
    }

    // 等待Server端回复
    int32 res = dp_msg->signal_event->WaitSignal(timeout);
    if (SIGNAL_EVENT_DONE != res) { // Wait signal失败或超时，未收到回复结果
      dp_msg->is_timeout = true;
      dp_msg->signal_event->ResetTriggerSignal();
      DLOG_ERROR(MOD_EB, "dp %p message %s[%d] %d Wait DP Replay failed", this, method.c_str(), dp_msg->message_id, timeout);

      chml::CritScope cs(&dp_req_crit_);
      request_map_.erase(dp_msg->message_id);
    } else {                        // 收到回复结果
      if (res_buffer) {
        *res_buffer = *dp_msg->res_buffer;
      }

      chml::CritScope cs(&dp_req_crit_);
      request_map_.erase(dp_msg->message_id);
      ret = dp_msg->result;
    }
  } while (0);
  PutSignal(dp_msg->signal_event);
  POINT_TIME_SPAN_IF(__start_time__, 3000, 9090, "method:%s,data:%s", method.c_str(), pdata);
  return ret;
}

bool DpClientImpl::SendDpReply(DpMessage::Ptr dp_msg) {
  if (dp_msg->type != TYPE_REQUEST) {
    DLOG_ERROR(MOD_EB, "Send DpReply failed, msg type %d is not REQUEST", dp_msg->type);
    return false;
  }
  if (dp_msg->is_timeout) {
    return false;
  }

  // 本地转发
  if (DpLocalDisp::Instance()) {
    bool bret = DpLocalDisp::Instance()->ProcessDpReply(shared_from_this(), dp_msg);
    // 本地消息才能本地处理signal
    if (bret) {
      return true;
    }
  }

  // 服务器转发
  if(0 != dp_srv_addr_.port()) {
    dp_msg->type = TYPE_REPLY;
    bool ret = SendDpMessage(dp_msg->message_id, TYPE_REPLY, TYPE_INVALID,
                             dp_msg->method, dp_msg->res_buffer->c_str(),
                             dp_msg->res_buffer->size(), dp_msg->session_id);
    if (!ret) {
      DLOG_ERROR(MOD_EB, "Transmit DP Replay to server failed");
      return false;
    }
  }
  return true;
}

int32 DpClientImpl::ListenMessage(const std::string method, uint32 type) {
  if ((0 == method.size())
      || ((TYPE_MESSAGE != type) && (TYPE_REQUEST != type))) {
    DLOG_ERROR(MOD_EB, "Add listen msg failed, invalid method:%s,type:%d", method.c_str(), type);
    return -1;
  }

  CritScope cs(&method_crit_);
  method_map_[method] = type;

  // 服务端地址为空,不进行服务端注册
  if (0 == dp_srv_addr_.port()) {
    DLOG_ERROR(MOD_EB, "server address is null:%s,type:%d", method.c_str(), type);
    return TYPE_SUCCEED;
  }

  int32 ret = TYPE_INVALID;
  DpMessage::Ptr dp_msg(new DpMessage);
  ASSERT_RETURN_FAILURE(!dp_msg, -1);
  do {
    dp_msg->signal_event = GetSignal();
    ASSERT_RETURN_FAILURE(!dp_msg->signal_event, ret);
    dp_msg->message_id = ++message_id_;
    dp_msg->method = method;
    dp_msg->session_id = (uint32)chml::XTimeUtil::ClockMSecond();
    dp_msg->type = TYPE_ADD_MESSAGE;
    dp_msg->is_timeout = false;
    dp_msg->result = TYPE_SUCCEED;
    request_map_[dp_msg->message_id] = dp_msg;
    bool res = SendDpMessage(dp_msg->message_id, TYPE_ADD_MESSAGE, type, method, NULL, 0, 0);
    if (!res) {
      DLOG_ERROR(MOD_EB, "Add listen msg %s failed", method.c_str());
      request_map_.erase(dp_msg->message_id);
      break;
    }

    int32 result = dp_msg->signal_event->WaitSignal(DP_CLI_REQUEST_TIMEOUT);
    if (SIGNAL_EVENT_DONE != result) { // Wait signal失败或超时，未收到回复结果
      dp_msg->is_timeout = true;
      dp_msg->signal_event->ResetTriggerSignal();
      DLOG_ERROR(MOD_EB, "Wait Replay error(res:%d), listen msg %s failed", result, method.c_str());
      request_map_.erase(dp_msg->message_id);
    } else {                           // 收到回复结果
      if (TYPE_SUCCEED == dp_msg->result) {
        DLOG_INFO(MOD_EB, "Add listen msg %s successed", method.c_str());
        request_map_.erase(dp_msg->message_id);
        ret = 0;
      } else {
        DLOG_INFO(MOD_EB, "Add listen msg %s failed, err:%d", method.c_str(), dp_msg->result);
        request_map_.erase(dp_msg->message_id);
      }
    }
  } while (0);

  PutSignal(dp_msg->signal_event);
  return ret;
}

void DpClientImpl::RemoveMessage(const std::string method) {
  if (0 == method.size()) {
    DLOG_ERROR(MOD_EB, "Remove listen msg failed, msg length: 0");
    return;
  }

  CritScope cs(&dp_req_crit_);
  method_map_.erase(method);

  // 服务端地址为空,不进行服务端取消注册
  if (0 == dp_srv_addr_.port()) {
    return;
  }

  uint32 message_id = ++message_id_;
  bool res = SendDpMessage(message_id, TYPE_REMOVE_MESSAGE, TYPE_INVALID, method, NULL, 0, 0);
  if (!res) {
    DLOG_ERROR(MOD_EB, "Remove listen msg %s failed", method.c_str());
  } else {
    DLOG_INFO(MOD_EB, "Remove listen msg %s successed", method.c_str());
  }
}

int32 DpClientImpl::LocalHandleRequest(DpClientImpl::Ptr dp_client, DpMessage::Ptr dp_msg) {
  chml::CritScope cr(&method_crit_);
  MSG_MAP::iterator pos = method_map_.find(dp_msg->method);
  if (pos != method_map_.end()) {
    dp_msg->type = pos->second;  // 发送端没有消息类型,只有接收端才知道消息类型
    msg_proc_evs_->Post(this, 0, dp_msg);
    if (TYPE_REQUEST == pos->second) {
      DLOG_DEBUG(MOD_EB, "local %p process request %s", msg_proc_evs_.get(), dp_msg->method.c_str());
      return pos->second;
    } else {
      DLOG_DEBUG(MOD_EB, "local %p process message %s", msg_proc_evs_.get(), dp_msg->method.c_str());
      return pos->second;
    }
  }
  return TYPE_INVALID;
}

///DpClient////////////////////////////////////////////////////////////////////
EventService::Ptr DpClient::DpClientEventService() {
  static EventService::Ptr one_proc_one_evs = EventService::Ptr();
  if (one_proc_one_evs == NULL) {
    one_proc_one_evs = EventService::CreateEventService(NULL, "dp_proc", 128 * 1024);
  }
  return one_proc_one_evs;
}

DpClient::Ptr DpClient::CreateDpClient(EventService::Ptr msg_proc_evs,
                                       const SocketAddress addr, const std::string &name) {
  if (NULL == msg_proc_evs) {
    DLOG_ERROR(MOD_EB, "Create DpClient failed, EventService::Ptr==NULL");
    return DpClient::Ptr();
  }

  DpClientImpl::Ptr client(new DpClientImpl(msg_proc_evs, name));
  if (client) {
    if (!client->Init(addr)) {
      client->ReConnect();
    }
    if (DpLocalDisp::Instance()) {
      DpLocalDisp::Instance()->AddDpClient(client);
    }
    return client;
  }

  return DpClient::Ptr();
}

DpClient::Ptr DpClient::CreateDpClient(EventService::Ptr msg_proc_evs,
                                       const SocketAddress addr) {
  return CreateDpClient(msg_proc_evs, addr, "unknow");
}


void DpClient::DestoryDpClient(DpClient::Ptr dp_client) {
  if (NULL == dp_client) {
    DLOG_ERROR(MOD_EB, "Destory DpClient failed, DpClient::Ptr==NULL");
    return;
  }

  DpClientImpl::Ptr client =
    boost::dynamic_pointer_cast<DpClientImpl>(dp_client);
  if (DpLocalDisp::Instance()) {
    DpLocalDisp::Instance()->RemoveDpClient(client);
  }
}

}  /* namespace chml */
