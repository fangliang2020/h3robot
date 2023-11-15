#include "eventservice/net/networkudpimpl.h"

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#else
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#endif

#include "eventservice/log/log_client.h"

#define TIMEOUT_TIMES 4
#ifdef WIN32
#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
void bzero(void* data, int size) {
  memset(data, 0, size);
}
void bcopy(void* des, const void* src, int size) {
  memcpy(des, src, size);
}
#endif

namespace chml {

AsyncUdpSocketImpl::AsyncUdpSocketImpl(EventService::Ptr es)
  : event_service_(es) {
}

AsyncUdpSocketImpl::~AsyncUdpSocketImpl() {
  DLOG_INFO(MOD_EB, "Destory socket implement");
  Close();
}

bool AsyncUdpSocketImpl::Init() {
  ASSERT_RETURN_FAILURE(socket_, false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(socket_event_, false);

  socket_ = event_service_->CreateSocket(UDP_SOCKET);
  ASSERT_RETURN_FAILURE(!socket_, false);
  socket_event_ = event_service_->CreateDispEvent(socket_,
                  DE_READ | DE_CLOSE);
  socket_event_->SignalEvent.connect(this, &AsyncUdpSocketImpl::OnSocketEvent);
  ASSERT_RETURN_FAILURE(!socket_event_, false);
  return true;
}

bool AsyncUdpSocketImpl::Init(const SocketAddress &addr) {
  ASSERT_RETURN_FAILURE(socket_, false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(socket_event_, false);

  socket_ = event_service_->CreateSocket(addr.family(), UDP_SOCKET);
  ASSERT_RETURN_FAILURE(!socket_, false);
  socket_event_ = event_service_->CreateDispEvent(socket_,
                  DE_READ | DE_CLOSE);
  socket_event_->SignalEvent.connect(this, &AsyncUdpSocketImpl::OnSocketEvent);
  ASSERT_RETURN_FAILURE(!socket_event_, false);
  return true;
}

bool AsyncUdpSocketImpl::Bind(const SocketAddress &bind_addr) {
  ASSERT_RETURN_FAILURE(!socket_, false);
  ASSERT_RETURN_FAILURE(!socket_event_, false);
  return socket_->Bind(bind_addr) != SOCKET_ERROR;
}

bool AsyncUdpSocketImpl::Bind(const SocketAddress &bind_addr, std::string iface){
  ASSERT_RETURN_FAILURE(!socket_, false);
  ASSERT_RETURN_FAILURE(!socket_event_, false);
  return socket_->Bind(bind_addr) != SOCKET_ERROR;
}

// Inherit with AsyncSocket
// Async write the data, if the operator complete, SignalWriteCompleteEvent
// will be called
bool AsyncUdpSocketImpl::SendTo(MemBuffer::Ptr buffer,
                                const SocketAddress &addr) {
  ASSERT_RETURN_FAILURE(!IsConnected(), false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(!socket_event_, false);

  BlocksPtr &blocks = buffer->blocks();

  for (BlocksPtr::iterator iter = blocks.begin();
       iter != blocks.end(); iter++) {
    Block::Ptr block = *iter;
    int res = socket_->SendTo(block->buffer, block->buffer_size, addr);
    if (res != block->buffer_size) {
      return false;
    }
  }
  return true;
}

bool AsyncUdpSocketImpl::AsyncRead() {
  ASSERT_RETURN_FAILURE(!socket_, false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(!socket_event_, false);
  socket_event_->AddEvent(DE_READ);
  return event_service_->Add(socket_event_);
}

// Returns the address to which the socket is bound.  If the socket is not
// bound, then the any-address is returned.
SocketAddress AsyncUdpSocketImpl::GetLocalAddress() const {
  return socket_->GetLocalAddress();
}

// Returns the address to which the socket is connected.  If the socket is
// not connected, then the any-address is returned.
SocketAddress AsyncUdpSocketImpl::GetRemoteAddress() const {
  return socket_->GetRemoteAddress();
}

void AsyncUdpSocketImpl::Close() {
  if (socket_) {
    socket_->Close();
    socket_.reset();
  }
  if (socket_event_) {
    event_service_->Remove(socket_event_);
    socket_event_->Close();
    socket_event_.reset();
  }
  RemoveAllSignal();
}

int AsyncUdpSocketImpl::GetError() const {
  ASSERT_RETURN_FAILURE(!socket_, 0);
  return socket_->GetError();
}

void AsyncUdpSocketImpl::SetError(int error) {
  ASSERT_RETURN_VOID(!socket_);
  return socket_->SetError(error);
}

bool AsyncUdpSocketImpl::IsConnected() {
  ASSERT_RETURN_FAILURE(!socket_, false);
  return socket_->GetState() != Socket::CS_CLOSED;
}

int AsyncUdpSocketImpl::GetOption(Option opt, int* value) {
  ASSERT_RETURN_FAILURE(!socket_, -1);
  return socket_->GetOption(opt, value);
}

int AsyncUdpSocketImpl::SetOption(Option opt, int value) {
  ASSERT_RETURN_FAILURE(!socket_, -1);
  return socket_->SetOption(opt, value);
}

int AsyncUdpSocketImpl::BindIface(std::string iface) {
  ASSERT_RETURN_FAILURE(!socket_, -1);
  DLOG_ERROR(MOD_EB, "BindIface %s", iface.c_str());
  int ret = socket_->SetOption(IPPROTO_IP, IP_MULTICAST_IF, (const char *)iface.c_str(), iface.length());
  printf("SetOption %d, %s", ret, strerror(errno));
  return ret;
}


///
void AsyncUdpSocketImpl::OnSocketEvent(EventDispatcher::Ptr accept_event,
                                       Socket::Ptr socket,
                                       uint32 event_type,
                                       int err) {
  AsyncUdpSocketImpl::Ptr live_this = shared_from_this();
  //DLOG_INFO(MOD_EB, "Event Type:%d", event_type);
  if(err || (event_type & DE_CLOSE)) {
    DLOG_ERROR(MOD_EB, "Close event");
    SocketErrorEvent(err);
    return ;
  }
  if (event_type & DE_WRITE) {
    socket_event_->RemoveEvent(DE_WRITE);
    //DLOG_INFO(MOD_EB, "Socket Write Event:%d", event_type);
    if (!socket_event_) {
      return ;
    }
  }
  if (event_type & DE_READ) {
    //DLOG_INFO(MOD_EB, "Socket Read Event:%d", event_type);
    SocketReadEvent();
  }
}

void AsyncUdpSocketImpl::SocketReadEvent() {
  MemBuffer::Ptr buffer = MemBuffer::CreateMemBuffer();
  SocketAddress   remote_addr;
  int res = socket_->RecvFrom(buffer, &remote_addr);
  int error_code = socket_->GetError();

  if (res > 0) {
    SocketReadComplete(buffer, remote_addr);
  } else if (res == 0) {
    //DLOG_ERROR(MOD_EB, "%s", evutil_socket_error_to_string(error_code));
    // Socket已经关闭了
    BOOST_ASSERT(error_code == 0);
    // SocketReadComplete(MemBuffer::Ptr(), error_code);
    SocketErrorEvent(error_code);
  } else {
    // 接收数据出问题了
    if (IsBlockingError(error_code)) {
      // 数据阻塞，之后继续接收数据
      return;
    } else {
      // Socket出了问题
      // SocketReadComplete(MemBuffer::Ptr(), error_code);
      SocketErrorEvent(error_code);
    }
  }
}

void AsyncUdpSocketImpl::SocketErrorEvent(int err) {
  AsyncUdpSocket::Ptr live_this = shared_from_this();
  SignalSocketErrorEvent(live_this, err);
  Close();
}

////////////////////////////////////////////////////////////////////////////////

void AsyncUdpSocketImpl::SocketReadComplete(MemBuffer::Ptr buffer,
    const SocketAddress &addr) {
  // 确保生命周期内不会因为外部删除而删除了整个对象
  // 如果是Socket Read事件导致的Socket错误，并不会出问题，
  // 但是如果在Write过程中出现了错误，那就会有问题了
  AsyncUdpSocketImpl::Ptr live_this = shared_from_this();
  SignalSocketReadEvent(live_this, buffer, addr);
}

////////////////////////////////////////////////////////////////////////////////


MulticastAsyncUdpSocket::MulticastAsyncUdpSocket(EventService::Ptr es)
  :AsyncUdpSocketImpl(es) {
}

bool MulticastAsyncUdpSocket::Bind(const SocketAddress &bind_addr) {
  SocketAddress multicast_bind_addr(INADDR_ANY, bind_addr.port());
  multicast_ip_addr_ = bind_addr.ipaddr().ToString();
  ChangeMulticastMembership(socket_, multicast_ip_addr_.c_str());
  return AsyncUdpSocketImpl::Bind(multicast_bind_addr);
}

bool MulticastAsyncUdpSocket::Bind(const SocketAddress &bind_addr, std::string iface) {
  //SocketAddress multicast_bind_addr(INADDR_ANY, bind_addr.port());
  SocketAddress multicast_bind_addr(bind_addr.ipaddr().ToString(), bind_addr.port());
  multicast_ip_addr_ = bind_addr.ipaddr().ToString();
  int ret = AsyncUdpSocketImpl::Bind(multicast_bind_addr);
  if (ret < 0) {
    DLOG_INFO(MOD_EB, "AsyncUdpSocketImpl::Bind erro:%d,%s\n",ret,strerror(errno));
    return false;
  }
#ifdef WIN32
  return ChangeMulticastMembership(socket_, multicast_ip_addr_.c_str()) == 0;
#else
  return ChangeMulticastMembership(socket_, multicast_ip_addr_.c_str(), iface) == 0;
#endif
}

bool MulticastAsyncUdpSocket::AsyncRead() {
  // ChangeMulticastMembership(socket_, multicast_ip_addr_.c_str(),bind_addr.port());
  return AsyncUdpSocketImpl::AsyncRead();
}

void MulticastAsyncUdpSocket::GetSupportMulticastInterface(std::vector<NetworkInterface> &ifs) {
#ifndef WIN32
  struct sockaddr_in *sin = NULL;
  struct ifaddrs *ifa = NULL, *ifList;

  if (getifaddrs(&ifList) < 0) {
    DLOG_INFO(MOD_EB, "Invoke getifaddrs failed!");
    return;
  }

  for (ifa = ifList; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_flags & IFF_UP &&
        !(ifa->ifa_flags & IFF_LOOPBACK) &&
        ifa->ifa_flags & IFF_MULTICAST &&
        ifa->ifa_addr->sa_family == AF_INET) {
      sin = (struct sockaddr_in *)ifa->ifa_addr;

      NetworkInterface nif;
      nif.if_name = ifa->ifa_name;
      nif.if_addr = inet_ntoa(sin->sin_addr);
      ifs.push_back(nif);

      DLOG_INFO(MOD_EB, "Interface name: %s, ip: %s", ifa->ifa_name,
                inet_ntoa(sin->sin_addr));
    }
  }

  freeifaddrs(ifList);
#endif
}

#ifdef WIN32

int MulticastAsyncUdpSocket::ChangeMulticastMembership(
  Socket::Ptr socket, const char* multicast_addr) {
  struct ip_mreq      mreq;
  bzero(&mreq, sizeof(struct ip_mreq));
  std::vector<NetworkInterfaceState> net_inter;
  UpdateNetworkInterface(net_inter);
  for (std::size_t i = 0; i < net_inter.size(); i++) {
    // Find network interface
    bool is_found = false;
    for (std::size_t j = 0; j < network_interfaces_.size(); j++) {
      if (network_interfaces_[j].adapter_name == net_inter[i].adapter_name
          && network_interfaces_[j].ip_addr.S_un.S_addr ==
          net_inter[i].ip_addr.S_un.S_addr
          && network_interfaces_[j].state == INTERFACE_STATE_ADD) {
        is_found = true;
        break;
      }
    }
    if (!is_found) {
      //DLOG_INFO(MOD_EB, "%s:", network_interfaces_[i].adapter_name);
      mreq.imr_interface.s_addr = net_inter[i].ip_addr.S_un.S_addr;
      mreq.imr_multiaddr.s_addr = inet_addr(multicast_addr);
      if (socket_->SetOption(IPPROTO_IP, IP_ADD_MEMBERSHIP,
                             (const char *)&mreq,
                             sizeof(struct ip_mreq)) == -1) {
        net_inter[i].state = INTERFACE_STATE_ERROR;
        //DLOG_ERROR(MOD_EB, "IP_ADD_MEMBERSHIP:%s", inet_ntoa(net_inter[i].ip_addr));
      } else {
        DLOG_INFO(MOD_EB, "IP_ADD_MEMBERSHIP:%s", inet_ntoa(net_inter[i].ip_addr));
        net_inter[i].state = INTERFACE_STATE_ADD;
      }
    }
    if (net_inter[i].state != INTERFACE_STATE_NONE) {
      continue;
    }
  }
  network_interfaces_ = net_inter;
  return 0;
}

int MulticastAsyncUdpSocket::UpdateNetworkInterface(
  std::vector<NetworkInterfaceState>& net_inter) {
  /* Declare and initialize variables */
  DWORD dwSize = 0;
  DWORD dwRetVal = 0;
  unsigned int i = 0;
  // Set the flags to pass to GetAdaptersAddresses
  ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
  // default to unspecified address family (both)
  ULONG family = AF_INET;
  LPVOID lpMsgBuf = NULL;
  PIP_ADAPTER_ADDRESSES pAddresses = NULL;
  ULONG outBufLen = WORKING_BUFFER_SIZE;
  ULONG Iterations = 0;
  PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
  PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
  PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
  PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
  IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
  IP_ADAPTER_PREFIX *pPrefix = NULL;
  // char temp_ip[128];
  do {
    pAddresses = (IP_ADAPTER_ADDRESSES *)MALLOC(outBufLen);
    if (pAddresses == NULL) {
      printf
      ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
      return 1;
    }
    dwRetVal =
      GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
      FREE(pAddresses);
      pAddresses = NULL;
    } else {
      break;
    }
    Iterations++;
  } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));
  // -------------------------------------------------------------------------
  if (dwRetVal == NO_ERROR) {
    // If successful, output some information from the data we received
    pCurrAddresses = pAddresses;
    while (pCurrAddresses) {
      //printf("\tAdapter name: %s\n", pCurrAddresses->AdapterName);
      pUnicast = pCurrAddresses->FirstUnicastAddress;
      if (pUnicast != NULL) {
        for (i = 0; pUnicast != NULL; i++) {
          if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
            NetworkInterfaceState nif;
            sockaddr_in* v4_addr =
              reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
            //printf("\tIpv4 pUnicast : %s\n", hexToCharIP(temp_ip, v4_addr->sin_addr));
            nif.ip_addr = v4_addr->sin_addr;
            nif.adapter_name = pCurrAddresses->AdapterName;
            nif.state = InterfaceState::INTERFACE_STATE_NONE;
            net_inter.push_back(nif);
          }
          pUnicast = pUnicast->Next;
        }
      }
      pCurrAddresses = pCurrAddresses->Next;
    }
  } else {
    if (dwRetVal != ERROR_NO_DATA) {
      if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                        |FORMAT_MESSAGE_FROM_SYSTEM
                        | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        dwRetVal,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        // Default language
                        (LPTSTR)& lpMsgBuf, 0, NULL)) {
        printf("\tError: %s", (const char *)lpMsgBuf);
        LocalFree(lpMsgBuf);
        if (pAddresses)
          FREE(pAddresses);
        return 1;
      }
    }
  }
  if (pAddresses) {
    FREE(pAddresses);
  }
  return 0;
}
#else
int MulticastAsyncUdpSocket::ChangeMulticastMembership(Socket::Ptr socket,
    const char* multicast_addr) {
  //printf("\n in AsyncUdpSocketImpl::Bind  %s, port:%d\n",  strerror(errno), port);
  std::vector<NetworkInterface> ifs;
  GetSupportMulticastInterface(ifs);

  if(ifs.size() > 0) {
    for(std::vector<NetworkInterface>::iterator iter = ifs.begin();
        iter != ifs.end(); ++iter) {
      NetworkInterface &nif = *iter;
      DLOG_INFO(MOD_EB, "Add interface %s:%s to multicast group",
                nif.if_name.c_str(),
                nif.if_addr.c_str());
      if(chml::HostNameIsV6(multicast_addr)) {
        struct ipv6_mreq    ipv6_mreq;
        //指定网卡
        ipv6_mreq.ipv6mr_interface = htonl(INADDR_ANY);
        bzero(&ipv6_mreq, sizeof(struct ipv6_mreq));
        inet_pton(AF_INET6, multicast_addr, &ipv6_mreq.ipv6mr_multiaddr.s6_addr);
        if (socket_->SetOption(IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                              (const char *)&ipv6_mreq,
                              sizeof(struct ipv6_mreq)) == -1) {
          DLOG_ERROR(MOD_EB, "Add membership error");
        }
      }
      else {
        struct ip_mreq      mreq;
        bzero(&mreq, sizeof(struct ip_mreq));
        mreq.imr_interface.s_addr = inet_addr(nif.if_addr.c_str());
        mreq.imr_multiaddr.s_addr = inet_addr(multicast_addr);
        if (socket_->SetOption(IPPROTO_IP, IP_ADD_MEMBERSHIP,
                              (const char *)&mreq,
                              sizeof(struct ip_mreq)) == -1) {
          DLOG_ERROR(MOD_EB, "Add membership error");
          //DLOG_ERROR(MOD_EB, "IP_ADD_MEMBERSHIP:%s", inet_ntoa(net_inter[i].ip_addr));
        }
      }
    } 
  }
  else {
    if(chml::HostNameIsV6(multicast_addr)){
      struct ipv6_mreq    ipv6_mreq;
      bzero(&ipv6_mreq, sizeof(struct ipv6_mreq));
      ipv6_mreq.ipv6mr_interface = htonl(INADDR_ANY);
      inet_pton(AF_INET6, multicast_addr, &ipv6_mreq.ipv6mr_multiaddr.s6_addr);
      if (socket_->SetOption(IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                            (const char *)&ipv6_mreq,
                            sizeof(struct ipv6_mreq)) == -1) {
        DLOG_ERROR(MOD_EB, "Add membership error");
      }
    }
    else {
      struct ip_mreq      mreq;
      bzero(&mreq, sizeof(struct ip_mreq));
      mreq.imr_interface.s_addr = INADDR_ANY;
      mreq.imr_multiaddr.s_addr = inet_addr(multicast_addr);
      if (socket_->SetOption(IPPROTO_IP, IP_ADD_MEMBERSHIP,
                            (const char *)&mreq,
                            sizeof(struct ip_mreq)) == -1) {
        DLOG_ERROR(MOD_EB, "Add membership error");
        //DLOG_ERROR(MOD_EB, "IP_ADD_MEMBERSHIP:%s", inet_ntoa(net_inter[i].ip_addr));
      }
    }
  }

  return 0;
}

int MulticastAsyncUdpSocket::ChangeMulticastMembership(Socket::Ptr socket,
                                                       const char *multicast_addr, 
                                                       std::string iface) {
  int ret = 0;
  struct ifreq ifr;
  struct ip_mreq mreq;
  struct ipv6_mreq ipv6_mreq;
  struct in_addr ifaddr;
  int sockfd = -1;

  if (iface.empty()) {
    return -1;
  }

  if ((sockfd = ::socket(AF_INET,SOCK_DGRAM,0)) < 0) {
    DLOG_ERROR(MOD_EB, "open socket failed, %s", strerror(errno));
    return -1;
  }

  memset(&ifr, 0x00, sizeof(ifr));
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);
  ret = ioctl(sockfd, SIOCGIFADDR, &ifr);
  if (ret < 0) {
    DLOG_ERROR(MOD_EB, "ioctl:SIOCGIFADDR failed, %s", strerror(errno));
    close(sockfd);
    return -1;
  }
  ifaddr = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr;
  DLOG_INFO(MOD_EB, "if addr: %s", inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));
  ret = ioctl(sockfd, SIOCGIFINDEX, &ifr);
  if (ret < 0) {
    DLOG_ERROR(MOD_EB, "ioctl:SIOCGIFADDR failed, %s", strerror(errno));
    close(sockfd);
    return -1;
  }
  DLOG_INFO(MOD_EB, "if index: %d, if name:%s", ifr.ifr_ifindex, iface.c_str());
  if(chml::HostNameIsV6(multicast_addr)) {
    //指定网卡
    ipv6_mreq.ipv6mr_interface = ifr.ifr_ifindex;
    bzero(&ipv6_mreq, sizeof(struct ipv6_mreq));
    inet_pton(AF_INET6, multicast_addr, &ipv6_mreq.ipv6mr_multiaddr.s6_addr);

    ret = socket_->SetOption(IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                          (const char *)&ipv6_mreq,
                          sizeof(struct ipv6_mreq));
    if (ret != 0) {
      DLOG_ERROR(MOD_EB, "Add membership error");
      return ret;
    }
    
    ret = socket_->SetOption(IPPROTO_IPV6,
                           IPV6_MULTICAST_IF,
                           (const char *)&ifr.ifr_ifindex,
                           sizeof(ifr.ifr_ifindex));
    if (ret != 0) {
      DLOG_ERROR(MOD_EB, "IPV6_MULTICAST_IF error %d, %s\n", ret, strerror(errno));
      return ret;
    }
  }
  else {
    bzero(&mreq, sizeof(struct ip_mreq));
    mreq.imr_interface = ifaddr;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_addr);

    ret = socket_->SetOption(IPPROTO_IP,
                            IP_ADD_MEMBERSHIP,
                            (const char *)&mreq,
                            sizeof(struct ip_mreq));

    if (ret != 0) {
      DLOG_ERROR(MOD_EB, "Add membership error %d, %s\n", ret, strerror(errno));
      return ret;
    }

    ret = socket_->SetOption(IPPROTO_IP,
                           IP_MULTICAST_IF,
                           (const char *)&ifaddr,
                           sizeof(struct in_addr));

    if (ret != 0) {
      DLOG_ERROR(MOD_EB, "IP_MULTICAST_IF error %d, %s\n", ret, strerror(errno));
      return ret;
    }
  }

  memset(&ifr, 0x00, sizeof(ifr));
  strncpy(ifr.ifr_name, iface.c_str(), iface.length());
  ret = socket_->SetOption(SOL_SOCKET, SO_BINDTODEVICE, (const char *)&ifr, sizeof(ifr));
  if (ret != 0) {
    DLOG_ERROR(MOD_EB, "SetOption SO_BINDTODEVICE %s %d, %s\n", iface.c_str(), ret, strerror(errno));
    return ret;
  }

  return 0;
}
#endif
}  // namespace chml
