#include "eventservice/net/eventservice.h"
#include "eventservice/net/networktinterfaceimpl.h"
#include "eventservice/net/networkudpimpl.h"
#include "eventservice/log/log_client.h"

namespace chml {

EventService::Ptr EventService::CreateEventService(Thread *thread,
    const std::string &thread_name,
    const size_t thread_stack_size) {
  Thread::Ptr p_thread(thread);
  EventService::Ptr es(new EventService(p_thread));
  es->InitEventService(thread_name, thread_stack_size);
  return es;
}

EventService::Ptr EventService::CreateCurrentEventService(
  const std::string &thread_name) {
  return CreateEventService(Thread::Current(), thread_name);
}

EventService::EventService(Thread::Ptr thread)
  : thread_(thread) {
}

EventService::~EventService() {
}

bool EventService::IsThisThread(Thread *thread) {
  return thread_.get() == thread;
}

bool EventService::SetThreadPriority(ThreadPriority priority) {
  return thread_->SetPriority(priority);
}

bool EventService::GetThreadAffinity(uint32 &cpu_affinity_mask) {
  return thread_->affinity(cpu_affinity_mask);
}
bool EventService::SetThreadAffinity(uint32 cpu_affinity_mask) {
  return thread_->SetAffinity(cpu_affinity_mask);
}

bool EventService::InitEventService(const std::string &thread_name,
                                    const size_t thread_stack_size) {
  if (thread_.get() == NULL) {
    thread_.reset(new Thread(NULL, thread_name, thread_stack_size));
    thread_->Start();
  } else {
    thread_->SetName(thread_name);
  }
  return true;
}

void EventService::Run() {
  thread_->Run();
}

void EventService::UninitEventService() {
  thread_->Stop();
}

void EventService::Post(MessageHandler *phandler,
                        uint32 id,
                        MessageData::Ptr pdata,
                        bool time_sensitive) {
  thread_->Post(phandler, id, pdata, time_sensitive);
}
void EventService::PostDelayed(int cmsDelay,
                               MessageHandler *phandler,
                               uint32 id,
                               MessageData::Ptr pdata) {
  thread_->PostDelayed(cmsDelay, phandler, id, pdata);
}

void EventService::Send(MessageHandler *phandler,
                        uint32 id,
                        MessageData::Ptr pdata) {
  thread_->Send(phandler, id, pdata);
}

void EventService::Clear(MessageHandler *phandler,
                         uint32 id,
                         MessageList* removed) {
  thread_->Clear(phandler, id, removed);
}

Socket::Ptr EventService::CreateSocket(int type) {
  ASSERT_RETURN_FAILURE(!thread_, Socket::Ptr());
  NetworkService *ss =
    dynamic_cast<NetworkService *>(thread_->socketserver());
  ASSERT_RETURN_FAILURE(ss == NULL, Socket::Ptr());
  return Socket::Ptr(ss->CreateSocket(type));
}

Socket::Ptr EventService::CreateSocket(int family, int type) {
  ASSERT_RETURN_FAILURE(!thread_, Socket::Ptr());
  NetworkService *ss =
    dynamic_cast<NetworkService *>(thread_->socketserver());
  ASSERT_RETURN_FAILURE(ss == NULL, Socket::Ptr());
  return Socket::Ptr(ss->CreateSocket(family, type));
}

EventDispatcher::Ptr EventService::CreateDispEvent(Socket::Ptr socket,
    uint32 enabled_events) {
  ASSERT_RETURN_FAILURE(!thread_, EventDispatcher::Ptr());
  NetworkService *ss =
    dynamic_cast<NetworkService *>(thread_->socketserver());
  ASSERT_RETURN_FAILURE(ss == NULL, EventDispatcher::Ptr());
  return EventDispatcher::Ptr(ss->CreateDispEvent(socket, enabled_events));
}

//Socket::Ptr EventService::WrapSocket(SOCKET s) {
//  ASSERT_RETURN_FAILURE(!thread_, Socket::Ptr());
//  NetworkService *ss =
//    dynamic_cast<NetworkService *>(thread_->socketserver());
//  ASSERT_RETURN_FAILURE(ss == NULL, Socket::Ptr());
//  return ss->WrapSocket(s);
//}

AsyncListener::Ptr EventService::CreateAsyncListener() {
  ASSERT_RETURN_FAILURE(!thread_, AsyncListener::Ptr());
  AsyncListener::Ptr async_lisener(new AsyncListenerImpl(shared_from_this()));
  return async_lisener;
}

AsyncConnecter::Ptr EventService::CreateAsyncConnect() {
  ASSERT_RETURN_FAILURE(!thread_, AsyncConnecter::Ptr());
  AsyncConnecter::Ptr async_connect(new AsyncConnecterImpl(shared_from_this()));
  return async_connect;
}

AsyncSocket::Ptr EventService::CreateAsyncSocket(Socket::Ptr socket) {
  ASSERT_RETURN_FAILURE(!thread_, AsyncSocket::Ptr());
  ASSERT_RETURN_FAILURE(!socket, AsyncSocket::Ptr());
  AsyncSocketImpl::Ptr async_socket(
    new AsyncSocketImpl(shared_from_this(), socket));
  if (async_socket->Init()) {
    return async_socket;
  }
  return AsyncSocket::Ptr();
}

AsyncSocket::Ptr EventService::CreateAsyncSocket(int socket_fd) {
  Socket::Ptr s(new PhysicalSocket(socket_fd));
  return CreateAsyncSocket(s);
}

AsyncUdpSocket::Ptr EventService::CreateUdpServer(
  const SocketAddress &bind_addr) {
  ASSERT_RETURN_FAILURE(!thread_, AsyncUdpSocket::Ptr());
  AsyncUdpSocketImpl::Ptr ausi(new AsyncUdpSocketImpl(shared_from_this()));
  if (!ausi->Init() || !ausi->Bind(bind_addr)) {
    DLOG_ERROR(MOD_EB, "Failure to init udp socket server");
    return AsyncUdpSocket::Ptr();
  }
  return ausi;
}

AsyncUdpSocket::Ptr EventService::CreateUdpClient() {
  ASSERT_RETURN_FAILURE(!thread_, AsyncUdpSocket::Ptr());
  AsyncUdpSocketImpl::Ptr ausi(new AsyncUdpSocketImpl(shared_from_this()));
  if (!ausi->Init()) {
    DLOG_ERROR(MOD_EB, "Failure to init udp socket server");
    return AsyncUdpSocket::Ptr();
  }
  return ausi;
}

AsyncUdpSocket::Ptr EventService::CreateMulticastSocket(
  const SocketAddress &multicast_addr) {
  ASSERT_RETURN_FAILURE(!thread_, AsyncUdpSocket::Ptr());
  AsyncUdpSocketImpl::Ptr ausi(new MulticastAsyncUdpSocket(shared_from_this()));
  if (!ausi->Init(multicast_addr) || !ausi->Bind(multicast_addr)) {
    DLOG_ERROR(MOD_EB, "Failure to init udp socket server");
    return AsyncUdpSocket::Ptr();
  }
  return ausi;
}

AsyncUdpSocket::Ptr EventService::CreateMulticastSocket(const SocketAddress &multicast_addr, std::string iface) {
  ASSERT_RETURN_FAILURE(!thread_, AsyncUdpSocket::Ptr());
  AsyncUdpSocketImpl::Ptr ausi(new MulticastAsyncUdpSocket(shared_from_this()));
  if (!ausi->Init(multicast_addr)){
    DLOG_ERROR(MOD_EB, "Failure to init udp socket server");
    return AsyncUdpSocket::Ptr();
  }

  ausi->SetOption(OPT_REUSEADDR, 1);
  if (!ausi->Bind(multicast_addr, iface)) {
    DLOG_ERROR(MOD_EB, "Failure to bind");
    return AsyncUdpSocket::Ptr();
  }
  return ausi;
}

int EventService::GetAsyncSocketCacheCnt() {
  return AsyncSocketImpl::GetTotalBlockCnt();
}

bool EventService::Add(EventDispatcher::Ptr socket) {
  ASSERT_RETURN_FAILURE(!thread_, false);
  NetworkService *ss =
    dynamic_cast<NetworkService *>(thread_->socketserver());
  ASSERT_RETURN_FAILURE(ss == NULL, false);
  ss->Add(socket);
  if (!thread_->IsCurrent()) {
    WakeUp();
  }
  return true;
}

bool EventService::Remove(EventDispatcher::Ptr socket) {
  ASSERT_RETURN_FAILURE(!thread_, false);
  NetworkService *ss =
    dynamic_cast<NetworkService *>(thread_->socketserver());
  ASSERT_RETURN_FAILURE(ss == NULL, false);
  ss->Remove(socket);
  if (!thread_->IsCurrent()) {
    WakeUp();
  }
  return true;
}

void EventService::WakeUp() {
  ASSERT_RETURN_VOID(!thread_);
  NetworkService *ss =
    dynamic_cast<NetworkService *>(thread_->socketserver());
  ASSERT_RETURN_VOID(ss == NULL);
  ss->WakeUp();
}

}  // namespace chml
