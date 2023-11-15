#ifndef EVENTSERVICE_DP_NET_CLIENT_H_
#define EVENTSERVICE_DP_NET_CLIENT_H_

#include "eventservice/net/eventservice.h"
#include "eventservice/event/signalevent.h"
#include "eventservice/dp/dpclient.h"
#include "eventservice/net/asyncpacketsocket.h"
#include "eventservice/log/log_client.h"

namespace chml {

class DpAgentManager;
/**
 * @brief DpClient处理类，当收到DpClient连接时，将创建此类对象，处理连接。
 * 从A进程发送的消息，系统不会再返回给A。因此DpClient会将自己的进程号，传递给此服务，此类保存客户端的进程类，以及DpClient的名称。
 * DpClient的名称最长为32字节，超过时，将会被截断
 */
class DpAgent : public boost::noncopyable,
  public boost::enable_shared_from_this<DpAgent>,
  public sigslot::has_slots<> {
 public:
  typedef boost::shared_ptr<DpAgent> Ptr;
  sigslot::signal2<DpAgent::Ptr, int> SignalErrorEvent;

 public:
  DpAgent(EventService::Ptr es);
  ~DpAgent();

  /**
   * @brief Start
   * @param socket 客户端Socket信息
   * @return
   */
  bool   Start(Socket::Ptr socket);

  uint32 GetId();
  uint32 GetPid();
#ifdef ENABLE_VTY
  void   DumpRegMessage(const void *vty_data);
#endif
  uint32 HandleDpMessage(DpAgent::Ptr src_agent, DpNetMsgHeader *dp_msg);

  /**
   * @brief 检测当前客户端是否注册了指定的消息
   * @param method
   * @param type
   * @return  若消息已经注册过了，但与type不一致，或者一致时，但type为REQUEST时，都将返回false
   */
  bool   CheckMessage(const std::string method, uint32 type);
  bool   SendMessage(uint32 msg_id, uint8 type, std::string method,
                     const char *data, uint32 data_size, uint32 session_id,
                     uint32 result = 0);

 private:
  void close ();
  void OnSocketWriteComplete(AsyncPacketSocket::Ptr async_socket);
  void OnSocketReadComplete(AsyncPacketSocket::Ptr async_socket, MemBuffer::Ptr data, uint16 flag);
  void OnSocketErrorEvent(AsyncPacketSocket::Ptr async_socket, int err);

  bool OnListenMessage(DpNetMsgHeader *dpmsg);
  bool OnRemoveMessage(DpNetMsgHeader *dpmsg);
  bool OnRequestMessage(DpNetMsgHeader *dpmsg);
  bool OnReplayMessage(DpNetMsgHeader *dpmsg);

  /**
   * @brief 设置客户端名称以及进程号。
   * @param dpmsg
   * @return 操作是否成功。需要注意的是：当客户端的名称为空时，系统被不会更新进程号以及名称，并且会返回true
   */
  bool OnSetClientName(DpNetMsgHeader *dpmsg);

 private:
  DpAgentManager          *agent_manager_;

 private:
  typedef std::map<std::string, uint32>   DP_MAP;
  DP_MAP                   method_map_;     // List of messages to listen for.

 private:
  AsyncPacketSocket::Ptr   packet_socket_;  // Tcp Socket
  EventService::Ptr        event_service_;  // Service线程
  uint32                   remote_pid_;     // 远端进程ID.从A接收到的消息,不返给A处理

 private:
  uint32                   agent_id_; // 系统为客户端分配的唯一标识
  std::string              agent_name_;
};

}

#endif  // EVENTSERVICE_DP_NET_CLIENT_H_

