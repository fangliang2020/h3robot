﻿#ifndef EVENTSERVICE_NET_NETWORKUDPINTERFACE_IMPLEMENT_H_
#define EVENTSERVICE_NET_NETWORKUDPINTERFACE_IMPLEMENT_H_

#include "eventservice/net/networktinterface.h"
#include "eventservice/net/eventservice.h"

namespace chml {

class AsyncUdpSocketImpl : public AsyncUdpSocket,
  public boost::noncopyable,
  public sigslot::has_slots<>,
  public boost::enable_shared_from_this<AsyncUdpSocketImpl> {
 public:
  typedef boost::shared_ptr<AsyncUdpSocketImpl> Ptr;

  virtual ~AsyncUdpSocketImpl();

  // Inherit with AsyncSocket
  // Async write the data, if the operator complete, SignalWriteCompleteEvent
  // will be called
  virtual bool SendTo(MemBuffer::Ptr buffer, const SocketAddress &addr);
  virtual bool AsyncRead();

  // Returns the address to which the socket is bound.  If the socket is not
  // bound, then the any-address is returned.
  virtual SocketAddress GetLocalAddress() const;

  // Returns the address to which the socket is connected.  If the socket is
  // not connected, then the any-address is returned.
  virtual SocketAddress GetRemoteAddress() const;

  virtual void Close();
  virtual int GetError() const;
  virtual void SetError(int error);
  virtual bool IsConnected();

  virtual int GetOption(Option opt, int* value);
  virtual int SetOption(Option opt, int value);
  virtual int BindIface(std::string iface);
 protected:
  AsyncUdpSocketImpl(EventService::Ptr es);
  
  virtual bool Init();
  virtual bool Init(const SocketAddress &addr);
  virtual bool Bind(const SocketAddress &bind_addr);
  virtual bool Bind(const SocketAddress &bind_addr, std::string iface);
  friend class EventService;

 private:
  void OnSocketEvent(EventDispatcher::Ptr accept_event, Socket::Ptr socket, uint32 event_type, int err);
  void SocketReadEvent();
  void SocketErrorEvent(int err);

  //
  void SocketReadComplete(MemBuffer::Ptr buffer, const SocketAddress &addr);
 protected:
  EventService::Ptr           event_service_;
  Socket::Ptr                 socket_;
  EventDispatcher::Ptr        socket_event_;
};

class MulticastAsyncUdpSocket : public AsyncUdpSocketImpl {
 public:
  typedef boost::shared_ptr<MulticastAsyncUdpSocket> Ptr;
#ifdef WIN32
  enum InterfaceState {
    INTERFACE_STATE_NONE,
    INTERFACE_STATE_ADD,
    INTERFACE_STATE_ERROR
  };
  struct NetworkInterfaceState {
    std::string adapter_name;
    IN_ADDR ip_addr;
    InterfaceState state;
  };
#endif
  struct NetworkInterface {
    std::string if_name;
    std::string if_addr;
  };
  virtual bool AsyncRead();
  virtual bool Bind(const SocketAddress &bind_addr);
  virtual bool Bind(const SocketAddress &bind_addr, std::string iface);
 protected:
  MulticastAsyncUdpSocket(EventService::Ptr es);
  friend class EventService;
 private:
  void GetSupportMulticastInterface(std::vector<NetworkInterface> &ifs);
  int ChangeMulticastMembership(Socket::Ptr socket, const char* multicast_addr, std::string iface);
  int ChangeMulticastMembership(Socket::Ptr socket, const char* multicast_addr);
#ifdef WIN32
  int UpdateNetworkInterface(std::vector<NetworkInterfaceState>& net_inter);
  std::vector<NetworkInterfaceState> network_interfaces_;
#endif
 private:
  std::string                        multicast_ip_addr_;
  std::string                        iface_;
};

////////////////////////////////////////////////////////////////////////////////

}  // namespace chml

#endif  // EVENTSERVICE_NET_NETWORKUDPINTERFACE_IMPLEMENT_H_
