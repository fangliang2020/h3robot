﻿#ifndef EVENTSERVICE_NET_NETWORKDINTERFACE_IMPLEMENT_H_
#define EVENTSERVICE_NET_NETWORKDINTERFACE_IMPLEMENT_H_

#include "eventservice/net/networktinterface.h"
#include "eventservice/net/eventservice.h"

namespace chml {

class AsyncSocketImpl : public MessageHandler,
  public AsyncSocket,
  public boost::noncopyable,
  public sigslot::has_slots<>,
  public boost::enable_shared_from_this<AsyncSocketImpl> {
 public:
  typedef boost::shared_ptr<AsyncSocketImpl> Ptr;

  virtual ~AsyncSocketImpl();

  // Inherit with AsyncSocket
  // Async write the data, if the operator complete, SignalWriteCompleteEvent
  // will be called
  virtual bool AsyncWrite(MemBuffer::Ptr buffer);
  virtual bool AsyncRead();

  // Returns the address to which the socket is bound.  If the socket is not
  // bound, then the any-address is returned.
  virtual SocketAddress GetLocalAddress() const;

  // Returns the address to which the socket is connected.  If the socket is
  // not connected, then the any-address is returned.
  virtual SocketAddress GetRemoteAddress() const;

  virtual void SetEncodeType(PACKET_ENCODE_TYPE encode_type);
  virtual void Close();
  virtual int GetError() const;
  virtual void SetError(int error);
  virtual bool IsConnected();

  virtual int GetOption(Option opt, int* value);
  virtual int SetOption(Option opt, int value);
  // 获取该AsyncSocket缓存的Membuffer block数量
  virtual int GetBlockCnt();
  // 获取所有AsyncSocket缓存的Membuffer block数量
  static  int GetTotalBlockCnt();
 protected:
  AsyncSocketImpl(EventService::Ptr es, Socket::Ptr s);
  bool Init();
  friend class EventService;
 private:
  virtual void OnMessage(chml::Message *msg);
  void OnSocketEvent(EventDispatcher::Ptr accept_event,
                     Socket::Ptr socket,
                     uint32 event_type,
                     int err);
  void SocketReadEvent();
  void SocketWriteEvent();
  void SocketErrorEvent(int err);

  //
  void SocketReadComplete(MemBuffer::Ptr buffer);
  void SocketWriteComplete();
  int32 TryToWriteData(bool is_emit_close_event, bool is_sync_call);
  void WaitToWriteData();
 private:
  static uint32         blocks_count_;     // 所有socket发送缓存的block总数量
  EventService::Ptr     event_service_;
  Socket::Ptr           socket_;
  EventDispatcher::Ptr  socket_event_;
  MemBuffer::Ptr        send_buffers_;     // 数据包发送缓存
  MemBuffer::Ptr        write_buffers_;    // 待发送数据包缓存
  PACKET_ENCODE_TYPE    encode_type_;      // 编码类型
  bool                  socket_writeable_; //
  CriticalSection       crit_;
};
//
class AsyncListenerImpl : public AsyncListener,
  public boost::noncopyable,
  public sigslot::has_slots<>,
  public boost::enable_shared_from_this<AsyncListenerImpl> {
 public:
  typedef boost::shared_ptr<AsyncListenerImpl> Ptr;
  virtual ~AsyncListenerImpl();

  // Inherit with AsyncLisener
  virtual bool Start(const SocketAddress addr, bool addr_reused);
  virtual void Close();
  virtual const SocketAddress BindAddress();
 protected:
  AsyncListenerImpl(EventService::Ptr es);
  friend class EventService;
 private:
  void OnAcceptEvent(EventDispatcher::Ptr accept_event,
                     Socket::Ptr socket,
                     uint32 event_type,
                     int err);
 private:
  EventService::Ptr           event_service_;
  Socket::Ptr                 socket_;
  EventDispatcher::Ptr   accept_event_;
  SocketAddress               listen_address_;
};

class  AsyncConnecterImpl : public AsyncConnecter,
  public boost::noncopyable,
  public sigslot::has_slots<>,
  public boost::enable_shared_from_this<AsyncConnecterImpl> {
 public:
  typedef boost::shared_ptr<AsyncConnecterImpl> Ptr;
  virtual ~AsyncConnecterImpl();
 public:
  virtual bool Connect(const SocketAddress addr, uint32 time_out);
  virtual void Close();
  virtual const SocketAddress ConnectAddress();
 protected:
  AsyncConnecterImpl(EventService::Ptr es);
  friend class EventService;
 private:
  void OnConnectEvent(EventDispatcher::Ptr accept_event,
                      Socket::Ptr socket,
                      uint32 event_type,
                      int err);
 private:
  EventService::Ptr      event_service_;
  Socket::Ptr            socket_;
  EventDispatcher::Ptr   connect_event_;
  SocketAddress          connect_address_;
};

////////////////////////////////////////////////////////////////////////////////

}  // namespace chml

#endif  // EVENTSERVICE_NET_NETWORKDINTERFACE_IMPLEMENT_H_
