/**
 * @Description: ntp
 * @Author:      zhz
 * @Version:     1
 */

#ifndef SRC_LIB_NTP_MANAGER_H_
#define SRC_LIB_NTP_MANAGER_H_

#include "net_server/base/base_module.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/event/messagehandler.h"

namespace net {

class NtpManager : public BaseModule,
                   public chml::MessageHandler {
 public:
  explicit NtpManager(NetServer *net_server_ptr);
  virtual ~NtpManager();

  int Start();
  int Stop();
 
 private:
  void OnMessage(chml::Message* msg);
  int Timing();
 
 private:
  chml::EventService::Ptr event_service_;
};

}

#endif  // SRC_LIB_NTP_MANAGER_H_