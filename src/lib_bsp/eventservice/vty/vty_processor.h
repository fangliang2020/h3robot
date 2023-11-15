#ifndef EVENTSERVICE_VTY_PROCESSOR_H_
#define EVENTSERVICE_VTY_PROCESSOR_H_

#include "eventservice/base/basictypes.h"
#include "eventservice/base/sigslot.h"
#include "eventservice/base/criticalsection.h"
#include "eventservice/log/log_client.h"
#include "boost/boost/boost_settings.hpp"

#include "eventservice/event/messagehandler.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/vty/vty_client.h"

#include "eventservice/vty/vty_common.h"

namespace vty {

struct VtyProcessorData : chml::MessageData {
  typedef boost::shared_ptr<VtyProcessorData> Ptr;
  VtyProcessorData() {
  }
  ~VtyProcessorData() {
  }
  std::string cmd_;
};

class VtyProcessor:
  public chml::MessageHandler,
  public boost::noncopyable,
  public sigslot::has_slots<>,
  public boost::enable_shared_from_this<VtyProcessor> {
 public:
  typedef boost::shared_ptr<VtyProcessor> Ptr;
  sigslot::signal2<VtyProcessor::Ptr, int> SignalErrorEvent;
  sigslot::signal2<VtyProcessor::Ptr, int> SignalConnectClose;

 public:
  VtyProcessor(chml::EventService::Ptr server_es,
               chml::AsyncSocket::Ptr async_socket,
               VtyCmdList<VtyCmdHandler>::Ptr cmds):
    server_es_(server_es),
    async_socket_ (async_socket),
    proc_done_(true),
    cmds_(cmds),
    started_(false),
    close_flag_(false) {
    DLOG_INFO(MOD_EB, "VtyProcessor constructor.");
  }
  ~VtyProcessor() {
    async_socket_->Close();
    DLOG_INFO(MOD_EB, "VtyProcessor Destory.");
  }

 public:
  // 开始处理数据
  int Start();
  // 拒绝
  int Reject();
  // 是否已经关闭连接
  bool IsClose();
  void Close();
  //return: 0成功 非0失败
  static int ParseCmd(const std::string &cmd,
                      char arg_buffer[VTY_CMD_MAX_TOT_LEN],
                      int &argc,
                      char *argv[VTY_CMD_MAX_PARAM_COUNT]);

 private:
//public:
  //SOCKET_EVENT
  void OnSocketWriteEvent(chml::AsyncSocket::Ptr async_socket);
  void OnSocketReadEvent(chml::AsyncSocket::Ptr async_socket,
                         chml::MemBuffer::Ptr data);
  void OnSocketErrorEvent(chml::AsyncSocket::Ptr async_socket, int err);
  //TELNET_EVENT;
  void OnTelnetReceiveData(telnet_event_t *ev);
  void OnTelnetSendData(telnet_event_t *ev);
  void OnTelnetIACCode(telnet_event_t *ev);

  void AddCmdToProcQueue(const char *buffer, size_t size);
  void ProcCmdFromProcQueue();
  std::string GetBriefString(const char *buffer, size_t size);

  static void VtyEventHander(telnet_t *telnet,
                             telnet_event_t *ev,
                             void *user_data);
  static void SysCmdQuit(const void *vty_data,
                         const void *user_data,
                         int argc, const char *argv[]);
  static void SysCmdList(const void *vty_data,
                         const void *user_data,
                         int argc, const char *argv[]);
  static void SysCmdHelp(const void *vty_data,
                         const void *user_data,
                         int argc, const char *argv[]);


  virtual void OnMessage(chml::Message *msg);

  //内建命令
  //int BuildinCmdShow();
  // int BuildinCmdQuit();
  // return 0:内建命令 1:非内建命令 -1:执行出错
  int ProcIfBuildinCmd(const int argc,
                       const char *argv[VTY_CMD_MAX_PARAM_COUNT]);
  void OnProcCmdMessage(VtyProcessorData::Ptr data);

 private:
  bool started_;						//开始标记，防止外部多次调用start
  chml::AsyncSocket::Ptr async_socket_;
  chml::EventService::Ptr server_es_, proc_es_;
  TelnetEntity::Ptr telnet_;
  std::queue<std::string> cmd_proc_queue_;
  VtyCmdList<VtyCmdHandler>::Ptr cmds_, sys_cmds_;

  volatile bool proc_done_;				//proc_es_ 线程任务状态标记
  volatile bool close_flag_;

};

} //namespace chml


#endif  // EVENTSERVICE_VTY_PROCESSOR_H_
