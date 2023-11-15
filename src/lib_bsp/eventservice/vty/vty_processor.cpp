#include "eventservice/vty/vty_processor.h"
#include "eventservice/vty/vty_server.h"
#include "eventservice/log/log_client.h"
#ifdef WIN32
#include <windows.h>
#elif POSIX
#endif

namespace  vty {

int VtyProcessor::Start() {
  ASSERT_RETURN_FAILURE(!async_socket_.get(), -1);
  ASSERT_RETURN_FAILURE(!server_es_.get(), -1);
  ASSERT_RETURN_FAILURE(started_, 0);

  telnet_ = TelnetEntity::Ptr(new TelnetEntity());
  ASSERT_RETURN_FAILURE(!telnet_.get(), -1);

  proc_es_ = chml::EventService::CreateEventService(NULL, "vty_proc");
  ASSERT_RETURN_FAILURE(!proc_es_.get(), -1);

  std::vector<telnet_telopt_t> tel_opts;
  tel_opts.push_back(telnet_telopt_t{ -1, 0, 0 });
  bool res = telnet_->Init(tel_opts, VtyEventHander, 0, this);
  ASSERT_RETURN_FAILURE(!res, -1);

  sys_cmds_ = VtyCmdList<VtyCmdHandler>::Ptr(new VtyCmdList<VtyCmdHandler>());
  ASSERT_RETURN_FAILURE(sys_cmds_.get() == NULL, -1);
  sys_cmds_->AddCmd("q", SysCmdQuit, this, "quit vty client");
  sys_cmds_->AddCmd("quit", SysCmdQuit, this, "quit vty client");
  sys_cmds_->AddCmd("exit", SysCmdQuit, this, "quit vty client");
  sys_cmds_->AddCmd("list", SysCmdList, this, "list user command");
  sys_cmds_->AddCmd("help", SysCmdHelp, this, "show all system command");
  sys_cmds_->AddCmd("h", SysCmdHelp, this, "show all system command");

  async_socket_->SignalSocketErrorEvent.connect(
    this, &VtyProcessor::OnSocketErrorEvent);
  async_socket_->SignalSocketReadEvent.connect(
    this, &VtyProcessor::OnSocketReadEvent);
  async_socket_->SignalSocketWriteEvent.connect(
    this, &VtyProcessor::OnSocketWriteEvent);
  async_socket_->AsyncRead();
  telnet_->Print("%s ", VTY_CMD_PREIFX);
  started_ = true;
  return 0;
}

int VtyProcessor::Reject() {
  return 0;
}

bool VtyProcessor::IsClose() {
  return telnet_->IsClose();
}

void VtyProcessor::Close() {
  DLOG_INFO(MOD_EB, "vty quit");
  telnet_->Close();
  server_es_->Post(this, MSG_ON_PROCESSOR_DISCONNECT);
}

void VtyProcessor::AddCmdToProcQueue(const char *buffer, size_t size) {
  if (cmd_proc_queue_.size() >= VTY_CMD_QUEUE_MAX_SIZE) {
    telnet_->Print("%s to many command, we ignore this command: %s...",
                   VTY_WARNNING_PREFIX,
                   GetBriefString(buffer, size).c_str());
    return;
  } else if (size >= VTY_CMD_MAX_TOT_LEN) {
    telnet_->Print("%s comand too long(max:%d, find:%d),"
                   " we ignore this command: %s...",
                   VTY_WARNNING_PREFIX,
                   VTY_CMD_MAX_TOT_LEN, size,
                   GetBriefString(buffer, size).c_str());
    return;
  } else if (size == 0) {
    DLOG_ERROR(MOD_EB, "command is empty, error.");
  }
  cmd_proc_queue_.push(std::string(buffer, size));
}

void VtyProcessor::ProcCmdFromProcQueue() {
  ASSERT_RETURN_VOID(proc_done_ == false);
  if (cmd_proc_queue_.empty()) {
    telnet_->Print("%s ", VTY_CMD_PREIFX);
    return;
  } else {
    std::string cmd = cmd_proc_queue_.front();
    cmd_proc_queue_.pop();

    VtyProcessorData::Ptr data(new VtyProcessorData());
    data->cmd_ = cmd;
    //telnet_->Print("%s %s", VTY_CMD_PREIFX, cmd.c_str());
    proc_done_ = false;
    proc_es_->Post(this, MSG_ON_PROC_CMD, data);
  }

  return;
}


std::string VtyProcessor::GetBriefString(const char *buffer, size_t size) {
  std::string ret;

  if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n') {
    size -= 2;
  } else if (buffer[size - 1] == '\n') {
    size -= 1;
  }
  int cp_size = ML_MIN(size, 8);
  ret.append(buffer, cp_size);
  return ret;
}

void VtyProcessor::OnTelnetReceiveData(telnet_event_t *ev) {
  DLOG_DEBUG(MOD_EB, "TELNET_EV_DATA");
  //telnet_->Print("cmd:%s", ev->data.buffer, ev->data.size);
  AddCmdToProcQueue(ev->data.buffer, ev->data.size);
  ProcCmdFromProcQueue();
}

void VtyProcessor::OnTelnetSendData(telnet_event_t *ev) {
  DLOG_DEBUG(MOD_EB, "TELNET_EV_SEND:(%s)", ev->data.buffer);
  async_socket_->AsyncWrite(ev->data.buffer, ev->data.size);
}

void VtyProcessor::OnTelnetIACCode(telnet_event_t *ev) {
  DLOG_DEBUG(MOD_EB, "TELNET_EV_Receive IAC:(%d)", ev->iac.cmd);
  if (ev->iac.cmd == 244) {
    Close();
    server_es_->Post(this, MSG_ON_PROCESSOR_DISCONNECT);
  }
}

void VtyProcessor::VtyEventHander(telnet_t *telnet,
                                  telnet_event_t *ev,
                                  void *user_data) {
  VtyProcessor::Ptr live_this =
    ((VtyProcessor*)(user_data))->shared_from_this();
  switch (ev->type) {
  /* data received */
  case TELNET_EV_DATA:
    live_this->OnTelnetReceiveData(ev);
    break;
  /* data must be sent */
  case TELNET_EV_SEND:
    live_this->OnTelnetSendData(ev);
    break;
  /* enable compress2 if accepted by client */
  case TELNET_EV_IAC:
    live_this->OnTelnetIACCode(ev);
    break;
  case TELNET_EV_DO:
    DLOG_DEBUG(MOD_EB, "TELNET_EV_DO");
    break;
  /* error */
  case TELNET_EV_ERROR:
    DLOG_DEBUG(MOD_EB, "TELNET_EV_ERROR");
    break;
  default:
    /* ignore */
    DLOG_DEBUG(MOD_EB, "TELNET_EV_TYPE UNKNOW %d", ev->type);
    break;
  }
}


void VtyProcessor::SysCmdQuit(const void *vty_data,
                              const void *user_data,
                              int argc, const char *argv[]) {
  ASSERT_RETURN_VOID(user_data == NULL);
  VtyProcessor::Ptr live_this =
    ((VtyProcessor*)(user_data))->shared_from_this();
  ASSERT_RETURN_VOID(live_this.get() == NULL);

  live_this->Close();
}

void VtyProcessor::SysCmdHelp(const void *vty_data,
                              const void *user_data,
                              int argc, const char *argv[]) {
  ASSERT_RETURN_VOID(user_data == NULL);
  VtyProcessor::Ptr live_this =
    ((VtyProcessor*)(user_data))->shared_from_this();
  ASSERT_RETURN_VOID(live_this.get() == NULL);

  live_this->telnet_->Print("system commands:\n");
  live_this->telnet_->Print(live_this->sys_cmds_->CmdsToString().c_str());
  //live_this->telnet_->Print("user commands:\n");
  //live_this->telnet_->Print(live_this->cmds_->CmdsToString().c_str());
}

void VtyProcessor::SysCmdList(const void *vty_data,
                              const void *user_data,
                              int argc, const char *argv[]) {
  ASSERT_RETURN_VOID(user_data == NULL);
  VtyProcessor::Ptr live_this =
    ((VtyProcessor*)(user_data))->shared_from_this();
  ASSERT_RETURN_VOID(live_this.get() == NULL);

  if (argc < 2) {
    //live_this->telnet_->Print("system commands:\n");
    //live_this->telnet_->Print(live_this->sys_cmds_->CmdsToString().c_str());
    live_this->telnet_->Print("user commands:\n");
    live_this->telnet_->Print(live_this->cmds_->CmdsToString().c_str());
  } else {
    std::list<std::string> list = live_this->cmds_->FindCmd(argv[1]);
    std::list<std::string>::iterator it;
    live_this->telnet_->Print("user commands:\n");
    for (it = list.begin(); it != list.end(); it++) {
      CMD_S<VtyCmdHandler> cmd_s;
      int ret = live_this->cmds_->GetCmdStruct(*it, cmd_s);
      if (ret != 0) {
        continue;
      }
      live_this->telnet_->Print("%s\n", cmd_s.ToHelpString().c_str());
    }
  }
}

void VtyProcessor::OnSocketWriteEvent(chml::AsyncSocket::Ptr async_socket) {
  DLOG_DEBUG(MOD_EB, "OnSocketWriteEvent");
}

void VtyProcessor::OnSocketReadEvent(chml::AsyncSocket::Ptr async_socket,
                                     chml::MemBuffer::Ptr data) {
  DLOG_DEBUG(MOD_EB, "OnSocketReadEvent");
  telnet_->Receive(data->ToString().c_str(), data->size());
  async_socket->AsyncRead();
}
void VtyProcessor::OnSocketErrorEvent(chml::AsyncSocket::Ptr async_socket,
                                      int err) {
  DLOG_DEBUG(MOD_EB, "OnSocketErrorEvent");
  Close();
  server_es_->Post(this, MSG_ON_PROCESSOR_DISCONNECT);
  //SignalErrorEvent(shared_from_this(), err);
}

void VtyProcessor::OnProcCmdMessage(VtyProcessorData::Ptr data) {
  ASSERT_RETURN_VOID(data.get() == NULL);
  int argc;
  char *argv[VTY_CMD_MAX_PARAM_COUNT];
  char arg_buffer[VTY_CMD_MAX_TOT_LEN];

  //exit(0);
  int ret = ParseCmd(data->cmd_, arg_buffer, argc, argv);
  if (ret != 0) {
    DLOG_DEBUG(MOD_EB, "Parse Cmd error. ret: %d. ", ret);
    return;
  }

  if (argc < 1 || argv[0] == NULL) {
    DLOG_DEBUG(MOD_EB, "cmd empty, ignore it.");
    return;
  }

  CMD_S<VtyCmdHandler> cmd_struct;

  //检测是否系统命令
  ret = sys_cmds_->GetCmdStruct(argv[0], cmd_struct);
  if (ret != 0 || cmd_struct.hander_ == NULL) {
    DLOG_DEBUG(MOD_EB, "no system handler match this command: %s",
               GetBriefString(data->cmd_.c_str(), data->cmd_.size()).c_str());
  } else {
    DLOG_DEBUG(MOD_EB, "proc thread start run task %s", cmd_struct.cmd_.c_str());
    cmd_struct.hander_(telnet_.get(), cmd_struct.user_data_, argc, (const char **)argv);
    DLOG_DEBUG(MOD_EB, "proc thread task finished %s", cmd_struct.cmd_.c_str());
    return;
  }

  //检测是否注册命令
  ret = cmds_->GetCmdStruct(argv[0], cmd_struct);
  if (ret != 0 || cmd_struct.hander_ == NULL) {
    DLOG_DEBUG(MOD_EB, "no user handler match this command: %s",
               GetBriefString(data->cmd_.c_str(), data->cmd_.size()).c_str());

    telnet_->Print("%s unknown command %s...\n",
                   VTY_WARNNING_PREFIX,
                   GetBriefString(data->cmd_.c_str(), data->cmd_.size()).c_str());
  } else {
    DLOG_DEBUG(MOD_EB, "proc thread start run task %s", cmd_struct.cmd_.c_str());
    cmd_struct.hander_(telnet_.get(), cmd_struct.user_data_, argc, (const char **)argv);
    DLOG_DEBUG(MOD_EB, "proc thread task finished %s", cmd_struct.cmd_.c_str());
  }

}

int VtyProcessor::ParseCmd(const std::string &cmd,
                           char arg_buffer[VTY_CMD_MAX_TOT_LEN],
                           int &argc,
                           char *argv[VTY_CMD_MAX_PARAM_COUNT]) {
  if (cmd.size() >= VTY_CMD_MAX_TOT_LEN) {
    DLOG_WARNING(MOD_EB, "cmd length exceed. max %d, find %d.",
                 VTY_CMD_MAX_TOT_LEN, cmd.size());
    return -1;
  }
  std::string s;
  char tmp[30];
  for (size_t i = 0; i < cmd.size(); i++) {
    sprintf(tmp, " %d", cmd[i]);
    s.append(tmp);
  }
  DLOG_DEBUG(MOD_EB, ".str:%s", s.c_str());

  int info_len;
  if (cmd.size() >= 2 && cmd[cmd.size() - 2] == '\r' && cmd[cmd.size() - 1] == '\n') {
    DLOG_DEBUG(MOD_EB, "cmd format normal. tail is \\r\\n, str:%s",
               cmd.c_str());
    info_len = cmd.size() - 2;
  } else if(cmd.size() >= 1 && cmd[cmd.size() - 1] == '\n') {
    DLOG_INFO(MOD_EB, "cmd format error. tail is \\n, not \\r\\n.str:%s",
              cmd.c_str());
    info_len = cmd.size() - 1;
  } else {
    DLOG_INFO(MOD_EB, "cmd format error. tail is not \\r\\n or \\n.str:%s.",
              cmd.c_str());
    info_len = cmd.size();
  }

  char *buf = arg_buffer;
  char *delim = NULL;
  memset(buf, 0, VTY_CMD_MAX_TOT_LEN * sizeof(char));
  memcpy(buf, cmd.c_str(), info_len);
  buf[info_len] = ' ';
  argc = 0;

  bool fi = true;
  do {
    if (!fi) {
      argv[argc++] = buf;
      *delim = '\0';
      buf = delim + 1;
    } else {
      fi = false;
    }
    while (*buf && (*buf == ' ')) { /* ignore spaces */
      buf++;
    }

    if (*buf == '\'') {
      buf++;
      delim = strchr(buf, '\'');
    } else {
      delim = strchr(buf, ' ');
    }
  } while (delim);

  if (argc == 0) {
    DLOG_DEBUG(MOD_EB, "black line.");
    return -1;
  }
  return 0;
}

void VtyProcessor::OnMessage(chml::Message *msg) {
  switch (msg->message_id) {
  case MSG_ON_PROCESSOR_DONE: {
    proc_done_ = true;
    if (IsClose()) {
      server_es_->Post(this, MSG_ON_PROCESSOR_DISCONNECT);
      return;
    }
    ProcCmdFromProcQueue();
    break;
  }
  case MSG_ON_PROCESSOR_DISCONNECT: {
    if (IsClose()) {
      async_socket_->Close();
    }
    if (IsClose() && proc_done_ == true) {
      SignalConnectClose(shared_from_this(), 0);
    }
    break;
  }
  case MSG_ON_PROC_CMD: {
    VtyProcessorData::Ptr data =
      boost::dynamic_pointer_cast<VtyProcessorData>(msg->pdata);

    OnProcCmdMessage(data);
    server_es_->Post(this, MSG_ON_PROCESSOR_DONE);
    break;
  }
  }
}

} // namespace vty