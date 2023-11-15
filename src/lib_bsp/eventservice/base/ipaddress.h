#ifndef EVENTSERVICES_BASE_IPADDRESS_H_
#define EVENTSERVICES_BASE_IPADDRESS_H_

#ifdef POSIX
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <string.h>
#include <string>
#include <vector>
#include "eventservice/base/basictypes.h"
#include "eventservice/base/byteorder.h"
#ifdef WIN32
#include "eventservice/base/win32.h"
#endif

namespace chml {

// Version-agnostic IP address class, wraps a union of in_addr and in6_addr.
class IPAddress {
 public:
  IPAddress() : family_(AF_UNSPEC) {
    ::memset(&u_, 0, sizeof(u_));
  }

  explicit IPAddress(const in_addr &ip4) : family_(AF_INET) {
    memset(&u_, 0, sizeof(u_));
    u_.ip4 = ip4;
  }

  explicit IPAddress(const in6_addr &ip6) : family_(AF_INET6) {
    u_.ip6 = ip6;
  }

  explicit IPAddress(uint32 ip_in_host_byte_order) : family_(AF_INET) {
    memset(&u_, 0, sizeof(u_));
    u_.ip4.s_addr = HostToNetwork32(ip_in_host_byte_order);
  }

  IPAddress(const IPAddress &other) : family_(other.family_) {
    ::memcpy(&u_, &other.u_, sizeof(u_));
  }

  ~IPAddress() {}

  const IPAddress & operator=(const IPAddress &other) {
    family_ = other.family_;
    ::memcpy(&u_, &other.u_, sizeof(u_));
    return *this;
  }

  bool operator==(const IPAddress &other) const;
  bool operator!=(const IPAddress &other) const;
  bool operator <(const IPAddress &other) const;
  bool operator >(const IPAddress &other) const;
  friend std::ostream& operator<<(std::ostream& os, const IPAddress& addr);

  int family() const {
    return family_;
  }
  in_addr ipv4_address() const;
  in6_addr ipv6_address() const;

  // Returns the number of bytes needed to store the raw address.
  size_t Size() const;

  // Wraps inet_ntop.
  std::string ToString() const;

  // Returns an unmapped address from a possibly-mapped address.
  // Returns the same address if this isn't a mapped address.
  IPAddress Normalized() const;

  // Returns this address as an IPv6 address.
  // Maps v4 addresses (as ::ffff:a.b.c.d), returns v6 addresses unchanged.
  IPAddress AsIPv6Address() const;

  // For socketaddress' benefit. Returns the IP in host byte order.
  uint32 v4AddressAsHostOrderInteger() const;

 private:
  int family_;
  union {
    in_addr ip4;
    in6_addr ip6;
  } u_;
};

bool IPFromAddrInfo(struct addrinfo* info, IPAddress* out);
bool IPFromString(const std::string& str, IPAddress* out);
bool IPIsAny(const IPAddress& ip);
bool IPIsLoopback(const IPAddress& ip);
bool IPIsPrivate(const IPAddress& ip);
bool IPIsUnspec(const IPAddress& ip);
size_t HashIP(const IPAddress& ip);

// These are only really applicable for IPv6 addresses.
bool IPIs6Bone(const IPAddress& ip);
bool IPIs6To4(const IPAddress& ip);
bool IPIsSiteLocal(const IPAddress& ip);
bool IPIsTeredo(const IPAddress& ip);
bool IPIsULA(const IPAddress& ip);
bool IPIsV4Compatibility(const IPAddress& ip);
bool IPIsV4Mapped(const IPAddress& ip);

// Returns the precedence value for this IP as given in RFC3484.
int IPAddressPrecedence(const IPAddress& ip);

// Returns 'ip' truncated to be 'length' bits long.
IPAddress TruncateIP(const IPAddress& ip, int length);
// Returns the number of contiguously set bits, counting from the MSB in network
// byte order, in this IPAddress. Bits after the first 0 encountered are not
// counted.
int CountIPMaskBits(IPAddress mask);
}  // namespace chml

#endif  // EVENTSERVICES_BASE_IPADDRESS_H_
