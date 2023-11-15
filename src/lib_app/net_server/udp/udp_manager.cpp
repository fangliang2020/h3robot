/**
 * @Description: udp
 * @Author:      zhz
 * @Version:     1
 */

#include <iostream>
#include <thread>
#include "json/json.h"
#include "udp_manager.h"
#include "eventservice/util/aes.h"
#include "net_server/main/net_server.h"
#include "net_server/base/jkey_common.h"

namespace net {

#define UDP_PORT         9000
#define BUFFER_SIZE      4096
#define AES_KEY          "__chml_rebot_h3_"

UdpManager::UdpManager(NetServer *net_server_ptr)
  : BaseModule(net_server_ptr) {
}

UdpManager::~UdpManager() {
}

int UdpManager::Start() {
  // TODO 还是要有个消息队列，要不要限制长度
  // mac需要
  auto func = [&]() {
    RunUdpServer();
  };
  
  std::thread udp_thread(func);
  udp_thread.detach();

  return 0;
}

int UdpManager::Stop() {
  return 0;
}

int UdpManager::SendMsg(std::string& msg, int sockfd, 
                        sockaddr_in& send_addr, socklen_t addr_len) {
  std::string send_body;
  aesEncryptStr(msg, send_body, (const uint8_t *)AES_KEY);
  int head = htobe32(send_body.size());
  char send_head[4] = {0};
  memcpy(send_head, &head, 4);
  std::string send_msg(send_head, 4);
  send_msg.append(send_body);

  int ret = sendto(sockfd, send_msg.c_str(), send_msg.size(), 
                   0, (struct sockaddr*)&send_addr, addr_len);

  std::cout << "UDP sendto " << inet_ntoa(send_addr.sin_addr) << ":" 
    << ntohs(send_addr.sin_port) << ", len:" << ret << std::endl;

  return ret;
}

void UdpManager::RecvDataHandle(const char* buffer, int len,
                                sockaddr_in& from_addr, sockaddr_in& send_addr) {
  if (len > 4) {
    int head = 0;
    memcpy(&head, buffer, 4);
    head = be32toh(head);
    printf("len : %d, head : %d\n", len, head);
    if (head <= len - 4) {
      std::string recv_data(buffer + 4, head);
      printf("recv_data : %s\n", recv_data.c_str());
      std::string dec_data;
      aesDecryptStr(recv_data, dec_data, (const uint8_t *)AES_KEY);
      printf("dec_data : %s\n", dec_data.c_str());
      Json::Value json_data;
      if (String2Json(dec_data, json_data)
          && json_data[JKEY_ID].isInt()
          && !json_data.isMember(JKEY_FROM)) {
        send_addr = from_addr;
        // TODO 设备自发现在这里处理，还是说全部发出去，感觉都可以

      }
    }
  }
}

// TODO 要不要做会话管理，先只给最后一个发吧，后面再看
int UdpManager::RunUdpServer() {
  int sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  if (sockfd < 0) {
    perror("sock error");
    return -1;
  }

  struct sockaddr_in server_addr, send_addr, from_addr;
  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htons(INADDR_ANY);
  server_addr.sin_port = htons(UDP_PORT);

  memset(&send_addr, 0, sizeof(struct sockaddr_in));
  send_addr.sin_family = AF_INET;
  send_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
  send_addr.sin_port = htons(UDP_PORT);

  while (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    perror("bind error");
    sleep(1);
  }

  char buffer[BUFFER_SIZE] = {0};
  int ret = -1;
  socklen_t from_len = sizeof(struct sockaddr_in);
  while (true) {
    // TODO 发消息
    // 发送使用 MemBuffer，地图可能会大得多
    {
      chml::CritScope lock(&cs_);
      if (!messages_.empty()) {
        SendMsg(messages_.front(), sockfd, send_addr, from_len);
        messages_.pop();
      }
    }

    // 收消息
    ret = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&from_addr, &from_len);
    if (ret > 0) {
      RecvDataHandle(buffer, ret, from_addr, send_addr);
    }
  }
  
  return 0;
}

}