#ifndef EVENTSERVICES_EVENTSERVICE_H_
#define EVENTSERVICES_EVENTSERVICE_H_


#include "eventservice/event/thread.h"
#include "eventservice/net/networkservice.h"
#include "eventservice/net/networktinterface.h"

namespace chml {

class EventService: public boost::noncopyable,
  public boost::enable_shared_from_this<EventService> {
 public:
  typedef boost::shared_ptr<EventService> Ptr;
 private:
  explicit EventService(Thread::Ptr thread);
 public:
  virtual ~EventService();
  // thread等于NULL时 thread_stack_size有效
  // thread_stack_size等于0时为系统默认线程栈大小
  // thread_stack_size小于系统最低栈大小时取最低栈大小值
  // 栈大小会向上取整为系统页大小整数倍(WIN 64KB;)
  static EventService::Ptr CreateEventService(Thread *thread = NULL,
      const std::string &thread_name = "ml_thread",
      const size_t thread_stack_size = 0);
  static EventService::Ptr CreateCurrentEventService(
    const std::string &thread_name = "ml_thread");

  bool IsThisThread(Thread *thread);
  bool SetThreadPriority(ThreadPriority priority);

  // 获取线程CPU亲和性(绑定CPU)
  // 结果为掩码，例如101代表可运行在第0,第2颗核心上
  // WIN平台,LITEOS 不支持,永远返回false;
  bool GetThreadAffinity(uint32 &cpu_affinity_mask);

  // 设置线程CPU亲和性(绑定CPU)
  // 结果为掩码，例如101代表可运行在第0,第2颗核心上
  // LITEOS 不支持，永远返回false
  bool SetThreadAffinity(uint32 cpu_affinity_mask);

  // Common function
  // thread等于NULL时 thread_stack_size有效
  // thread_stack_size等于0时为系统默认线程栈大小
  // thread_stack_size小于系统最低栈大小时取最低栈大小值
  // 栈大小会向上取整为系统页大小整数倍(WIN 64KB;)
  bool InitEventService(const std::string &thread_name = "ml_thread",
                        const size_t thread_stack_size = 0);
  void Run();
  void UninitEventService();

  size_t size() {
    if (thread_.get()) {
      return thread_->size();
    }

    printf("----- thread is null -----\n");
    return 0;
  }

  // 发送异步消息
  void Post(MessageHandler *phandler,
            uint32 id = 0,
            MessageData::Ptr pdata = MessageData::Ptr(),
            bool time_sensitive = false);

  // 发送定时器消息
  void PostDelayed(int cmsDelay,
                   MessageHandler *phandler,
                   uint32 id = 0,
                   MessageData::Ptr pdata = MessageData::Ptr());

  // 发送同步消息
  void Send(MessageHandler *phandler,
            uint32 id = 0,
            MessageData::Ptr pdata = MessageData::Ptr());

  // 删除phandler或id指定的消息，如果phandler == NULL && id == MQID_ANY，
  // 则删除该EventService对应线程所有未处理的消息, 包括：post、post delayed、
  // send消息。MessageList* removed：[OUT]所有被删除的消息
  void Clear(MessageHandler *phandler,
             uint32 id = MQID_ANY,
             MessageList* removed = NULL);

  // 网络相关的服务
  virtual Socket::Ptr CreateSocket(int type);
  virtual Socket::Ptr CreateSocket(int family, int type);

  virtual EventDispatcher::Ptr CreateDispEvent(Socket::Ptr socket,
      uint32 enabled_events);

  AsyncListener::Ptr    CreateAsyncListener();
  AsyncConnecter::Ptr   CreateAsyncConnect();
  // 在当前线程创建一个AsyncSocket
  // socket: 已经连接的Socket指针
  // return: 成功返回建立的AsyncSocket, 失败返回空指针
  AsyncSocket::Ptr      CreateAsyncSocket(Socket::Ptr socket);
  // 在当前线程创建一个AsyncSocket
  // socket_fd: 已经建立好连接的套接字描述符
  // return: 成功返回建立的AsyncSocket, 失败返回空指针
  AsyncSocket::Ptr      CreateAsyncSocket(int socket_fd);
  AsyncUdpSocket::Ptr   CreateUdpServer(const SocketAddress &bind_addr);
  AsyncUdpSocket::Ptr   CreateUdpClient();
  AsyncUdpSocket::Ptr   CreateMulticastSocket(
    const SocketAddress &multicast_addr);
  AsyncUdpSocket::Ptr   CreateMulticastSocket(const SocketAddress &multicast_addr, std::string eth);
  // 获取所有AsyncSocket缓存的Membuffer block数量
  static int GetAsyncSocketCacheCnt();

  // Socket::Ptr WrapSocket(SOCKET s);
  virtual bool Add(EventDispatcher::Ptr socket);
  virtual bool Remove(EventDispatcher::Ptr socket);

  void InitLocalSharedData(MessageData::Ptr data) {
    local_shared_data_crit_.Enter();
    local_shared_data_ = std::move(data);
    local_shared_data_crit_.Leave();
  }
  MessageData::Ptr GetLocalSharedData() {
    local_shared_data_crit_.Enter();
    return local_shared_data_;
  }
  void PutLocalSharedData(MessageData::Ptr data) {
    local_shared_data_ = std::move(data);
    local_shared_data_crit_.Leave();
  }
  //
  void WakeUp();
 private:
  Thread::Ptr thread_;
  CriticalSection local_shared_data_crit_;
  MessageData::Ptr local_shared_data_;
};

}  // namespace chml

#endif  // EVENTSERVICES_EVENTSERVICE_H_
