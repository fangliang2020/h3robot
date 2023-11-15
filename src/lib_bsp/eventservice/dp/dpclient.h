#ifndef EVENTSERVICE_DP_DPCLIENT_H_
#define EVENTSERVICE_DP_DPCLIENT_H_

#include "eventservice/net/eventservice.h"
#include "eventservice/event/signalevent.h"

namespace chml {

#ifndef ML_DP_SERVER_HOST
#define ML_DP_SERVER_HOST "127.0.0.1"
#endif  // ML_DP_SERVER_HOST

#define ML_DP_SERVER_PORT (6101)

typedef std::string DpBuffer;

enum {
  TYPE_INVALID = -1,
  TYPE_MESSAGE = 0,
  TYPE_REQUEST = 1,
  TYPE_REPLY = 2,
  TYPE_ERROR_TIMEOUT = 3,
  TYPE_ERROR_FORMAT = 4,
  TYPE_GET_SESSION_ID = 5,
  TYPE_SUCCEED = 6,
  TYPE_FAILURE = 7,
  TYPE_ADD_MESSAGE = 8,
  TYPE_REMOVE_MESSAGE = 9,
  TYPE_ERROR_DISCONNECTED = 10,
  TYPE_SET_NAME = 11,
};

struct DpMessage : public MessageData {
  DpMessage() {
  }
  ~DpMessage() {
    signal_event.reset();
  }
  typedef boost::shared_ptr<DpMessage> Ptr;
  uint32        type;        // 消息类型
  std::string   method;      // DP消息描述
  uint32        session_id;  // 会话ID: 暂时用作消息
  uint32        message_id;  // 消息ID: high 16 bits: agent id; low 16 bits: message id, user defined.
  DpBuffer      req_buffer;  // Request数据，Request端填写
  DpBuffer*     res_buffer;  // Replay数据，Replay端`填写
  DpBuffer      tmp_res;     // 临时变量，兼容之前的用法
  bool          is_timeout;  // Replay超时标识
  uint32        result;      //
  SignalEvent::Ptr  signal_event;
};

typedef struct {
  uint8         type;        // 消息类型
  uint8         listen_type; // 监听消息类型，用于ListenMessage
  uint16        method_size; // Dp method长度
  uint32        process_id;  // 进程ID; 区分
  uint32        message_id;  // 消息ID: high 16 bits: agent id; low 16 bits: message id, user defined.
  uint32        session_id;  // DP消息Session ID
  uint32        result;      //
  uint32        data_size;   // DP消息用户数据长度
  char          body[0];     // 数据：method + data
} DpNetMsgHeader;

class DpClient {
 public:
  typedef boost::shared_ptr<DpClient> Ptr;
  // 回调函数，请求、广播消息回调函数
  sigslot::signal2<DpClient::Ptr, DpMessage::Ptr> SignalDpMessage;
  // 回调函数，错误事件回调函数
  sigslot::signal2<DpClient::Ptr, int> SignalErrorEvent;

 public:
  virtual ~DpClient() {};

  // 创建一个本地DpClient对象
  // es:EventService，指定处理DP消息时运行的线程上下文
  // return 成功，！= NULL; 失败，NULL
  static DpClient::Ptr CreateDpClient(EventService::Ptr msg_proc_evs, const SocketAddress addr,
                                      const std::string &name);
  static DpClient::Ptr CreateDpClient(EventService::Ptr msg_proc_evs, const SocketAddress addr);

  // 删除一个DpClient对象，程序退出的时候，应该删除这个对象
  // dp_client: DpClient对象
  static void DestoryDpClient(DpClient::Ptr dp_client);

  // 一个进程一个EventService,用于DP Send|Recv网络数据
  static EventService::Ptr DpClientEventService();

  // 发送一个星形结构消息，回复结果通过指针res_buffer返回，
  // 该接口为阻塞调用，直到收到回复消息或者超时时返回
  // method: 消息描述
  // session_id：会话id，用户自定义
  // data: 消息内容
  // data_size: 消息内容长度（Byte）
  // res_buffer: 输出参数，由Request端提供内存，Replay端填写回复消息内容
  // timeout_millisecond: 超时时常(单位：ms)，默认为5秒钟
  // return > 0: 发送成功，接收到该消息的client数量
  //        = 0: 没有client监听该消息(自身client除外)
  //        -1: 参数错误、或者对端回复超时
  virtual int SendDpMessage(const std::string method, uint32 session_id, const char *data,
                            uint32 data_size, DpBuffer *res_buffer, uint32 timeout = 5000) = 0;


  virtual int SendRemoteMessage(const std::string method, uint32 type, uint32 session_id, const char *data,
                            uint32 data_size, DpBuffer *res_buffer, uint32 timeout = 5000) = 0;
  
  // 回复一个星形结构消息，收到DP请求后，直接填写回复内容到请求
  // DpMessage中的res_buffer, res_buffer内存由request端提供
  // dp_msg：dp消息指针，即DP Request收到的dp消息指针
  // return 成功，true；失败，false
  virtual bool SendDpReply(DpMessage::Ptr dp_msg) = 0;

  // 注册一个监听消息
  // method：dp 消息描述
  // type:消息类型，TYPE_REQUEST、TYPE_MESSAGE
  // return 成功: 0; 失败: -1
  virtual int ListenMessage(const std::string method, uint32 type) = 0;

  // 取消一个消息监听
  // method：dp 消息描述
  virtual void RemoveMessage(const std::string method) = 0;
};

}

#endif  // EVENTSERVICE_DP_DPCLIENT_H_

