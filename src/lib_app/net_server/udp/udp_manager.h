/**
 * @Description: udp
 * @Author:      zhz
 * @Version:     1
 */

#ifndef SRC_LIB_UDP_MANAGER_H_
#define SRC_LIB_UDP_MANAGER_H_

#include "net_server/base/base_module.h"
#include "eventservice/net/eventservice.h"

namespace net {

class UdpManager : public BaseModule {
 public:
  explicit UdpManager(NetServer *net_server_ptr);
  virtual ~UdpManager();

  int Start();
  int Stop();
 
 private:
  int RunUdpServer();
  int SendMsg(std::string& msg, int sockfd, 
              sockaddr_in& send_addr, socklen_t addr_len);
  void RecvDataHandle(const char* buffer, int len,
                      sockaddr_in& from_addr, sockaddr_in& send_addr);
 
 private:
  std::queue<std::string> messages_;
  chml::CriticalSection cs_;
};

}

#endif  // SRC_LIB_WIFI_MANAGER_H_
