﻿#ifndef EVENTSERVICES_BASE_SOCKET_H__
#define EVENTSERVICES_BASE_SOCKET_H__

#include <errno.h>

#ifdef POSIX
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define SOCKET_EACCES EACCES
#endif

#ifdef WIN32
#include "eventservice/base/win32.h"
#endif

#include "eventservice/base/basictypes.h"
#include "eventservice/base/socketaddress.h"
#include "eventservice/base/constructormagic.h"
#include "eventservice/mem/membuffer.h"
#include "eventservice/base/sigslot.h"

// Rather than converting errors into a private namespace,
// Reuse the POSIX socket api errors. Note this depends on
// Win32 compatibility.

#ifdef WIN32
#undef EWOULDBLOCK  // Remove errno.h's definition for each macro below.
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#undef EALREADY
#define EALREADY WSAEALREADY
#undef ENOTSOCK
#define ENOTSOCK WSAENOTSOCK
#undef EDESTADDRREQ
#define EDESTADDRREQ WSAEDESTADDRREQ
#undef EMSGSIZE
#define EMSGSIZE WSAEMSGSIZE
#undef EPROTOTYPE
#define EPROTOTYPE WSAEPROTOTYPE
#undef ENOPROTOOPT
#define ENOPROTOOPT WSAENOPROTOOPT
#undef EPROTONOSUPPORT
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
#undef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
#undef EOPNOTSUPP
#define EOPNOTSUPP WSAEOPNOTSUPP
#undef EPFNOSUPPORT
#define EPFNOSUPPORT WSAEPFNOSUPPORT
#undef EAFNOSUPPORT
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#undef EADDRINUSE
#define EADDRINUSE WSAEADDRINUSE
#undef EADDRNOTAVAIL
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#undef ENETDOWN
#define ENETDOWN WSAENETDOWN
#undef ENETUNREACH
#define ENETUNREACH WSAENETUNREACH
#undef ENETRESET
#define ENETRESET WSAENETRESET
#undef ECONNABORTED
#define ECONNABORTED WSAECONNABORTED
#undef ECONNRESET
#define ECONNRESET WSAECONNRESET
#undef ENOBUFS
#define ENOBUFS WSAENOBUFS
#undef EISCONN
#define EISCONN WSAEISCONN
#undef ENOTCONN
#define ENOTCONN WSAENOTCONN
#undef ESHUTDOWN
#define ESHUTDOWN WSAESHUTDOWN
#undef ETOOMANYREFS
#define ETOOMANYREFS WSAETOOMANYREFS
#undef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT
#undef ECONNREFUSED
#define ECONNREFUSED WSAECONNREFUSED
#undef ELOOP
#define ELOOP WSAELOOP
#undef ENAMETOOLONG
#define ENAMETOOLONG WSAENAMETOOLONG
#undef EHOSTDOWN
#define EHOSTDOWN WSAEHOSTDOWN
#undef EHOSTUNREACH
#define EHOSTUNREACH WSAEHOSTUNREACH
#undef ENOTEMPTY
#define ENOTEMPTY WSAENOTEMPTY
#undef EPROCLIM
#define EPROCLIM WSAEPROCLIM
#undef EUSERS
#define EUSERS WSAEUSERS
#undef EDQUOT
#define EDQUOT WSAEDQUOT
#undef ESTALE
#define ESTALE WSAESTALE
#undef EREMOTE
#define EREMOTE WSAEREMOTE
#undef EACCES
#define SOCKET_EACCES WSAEACCES
#endif  // WIN32

#ifdef POSIX

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

#ifndef closesocket
#define closesocket(s) close(s)
#endif  // closesocket

#endif  // POSIX

#define TCP_SOCKET SOCK_STREAM
#define UDP_SOCKET SOCK_DGRAM

namespace chml {

inline bool IsBlockingError(int e) {
  return (e == EWOULDBLOCK) || (e == EAGAIN) || (e == EINPROGRESS);
}

enum Option {
  OPT_DONTFRAGMENT,
  OPT_RCVBUF,      // receive buffer size
  OPT_SNDBUF,      // send buffer size
  OPT_NODELAY,     // whether Nagle algorithm is enabled
  OPT_IPV6_V6ONLY, // Whether the socket is IPv6 only.
  OPT_MULTICAST_MEMBERSHIP,
  OPT_MULTICAST_LOOP,  // Whether UDP multicast messages support loopback.
  OPT_REUSEADDR    // reuse address.
};

// General interface for the socket implementations of various networks.  The
// methods match those of normal UNIX sockets very closely.
class Socket : public boost::noncopyable,
  public boost::enable_shared_from_this<Socket> {
 public:
  typedef boost::shared_ptr<Socket> Ptr;
  sigslot::signal1<Socket::Ptr> SignalReadEvent;        // ready to read
  sigslot::signal1<Socket::Ptr> SignalWriteEvent;       // ready to write
  sigslot::signal1<Socket::Ptr> SignalConnectEvent;     // connected
  sigslot::signal2<Socket::Ptr, int> SignalCloseEvent;  // closed
 public:
  Socket() {};
  virtual ~Socket() {};

  virtual Socket::Ptr Accept(SocketAddress* paddr) = 0;


  // Returns the address to which the socket is bound.  If the socket is not
  // bound, then the any-address is returned.
  virtual SocketAddress GetLocalAddress() const = 0;

  // Returns the address to which the socket is connected.  If the socket is
  // not connected, then the any-address is returned.
  virtual SocketAddress GetRemoteAddress() const = 0;

  virtual int Bind(const SocketAddress& addr) = 0;
  virtual int Connect(const SocketAddress& addr) = 0;
  virtual int Send(const void *pv, size_t cb) = 0;
  virtual int Send(MemBuffer::Ptr buffer) = 0;
  virtual int SendTo(const void *pv, size_t cb, const SocketAddress& addr) = 0;
  virtual int Recv(void *pv, size_t cb) = 0;
  virtual int Recv(MemBuffer::Ptr buffer) = 0;
  virtual int RecvFrom(void *pv, size_t cb, SocketAddress *paddr) = 0;
  virtual int RecvFrom(MemBuffer::Ptr buffer, SocketAddress *out_addr) = 0;
  virtual int Listen(int backlog) = 0;
  // virtual Socket *Accept(SocketAddress *paddr) = 0;
  virtual int Close() = 0;
  virtual int GetError() const = 0;
  virtual void SetError(int error) = 0;
  inline bool IsBlocking() const {
    return IsBlockingError(GetError());
  }

  //void UnbindAllSignal() {
  //  SignalReadEvent.disconnect_all();
  //  SignalWriteEvent.disconnect_all();
  //  SignalConnectEvent.disconnect_all();
  //  SignalCloseEvent.disconnect_all();
  //}

  enum ConnState {
    CS_CLOSED,
    CS_CONNECTING,
    CS_CONNECTED
  };
  virtual ConnState GetState() const = 0;

  // Fills in the given uint16 with the current estimate of the MTU along the
  // path to the address to which this socket is connected. NOTE: This method
  // can block for up to 10 seconds on Windows.
  // virtual int EstimateMTU(uint16* mtu) = 0;

  virtual int GetOption(Option opt, int* value) = 0;
  virtual int SetOption(Option opt, int value) = 0;
  virtual int SetOption(int level,
                        int optname,
                        const char *value,
                        int optlen) = 0;
};

}  // namespace chml

#endif  // EVENTSERVICES_BASE_SOCKET_H__
