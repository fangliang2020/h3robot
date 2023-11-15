#include "eventservice/vty/vty_common.h"


namespace vty {

/*----------------------------telnetTntity-----------------------------------*/

bool TelnetEntity::Init(std::vector<telnet_telopt_t> telopts,
                        telnet_event_handler_t handler,
                        unsigned char flags, void *user_data) {
  telopts_ =
    (telnet_telopt_t *)malloc(sizeof(telnet_telopt_t) * telopts.size());
  ASSERT_RETURN_FAILURE(telopts_ == NULL, false);
  for (size_t i = 0; i < telopts.size(); i++) {
    telopts_[i] = telopts[i];
  }

  telnet_ = telnet_init(telopts_, handler, flags, user_data);
  return telnet_ != NULL;
};

int TelnetEntity::Receive(const char *buffer, size_t size) {
  chml::CritScope crit(&crit_);
  //ASSERT_RETURN_FAILURE(IsClose(), 0);
  if (IsClose()) {
    return 0;	//连接关闭后不再执行任何操作
  }
  ASSERT_RETURN_FAILURE(telnet_ == NULL, -1);
  telnet_recv(telnet_, buffer, size);
  return 0;
}

int TelnetEntity::Print(const char *fmt, ...) {
  chml::CritScope crit(&crit_);
  //ASSERT_RETURN_FAILURE(IsClose(), 0);
  if (IsClose()) {
    return 0;	//连接关闭后不再执行任何操作
  }

  va_list va;
  int rs;

  va_start(va, fmt);
  rs = telnet_vprintf(telnet_, fmt, va);
  va_end(va);

  return rs;
}

int TelnetEntity::VPrint(const char *fmt, va_list va) {
  chml::CritScope crit(&crit_);
  //ASSERT_RETURN_FAILURE(IsClose(), 0);
  if (IsClose()) {
    return 0;	//连接关闭后不再执行任何操作
  }
  return telnet_vprintf(telnet_, fmt, va);
}
void TelnetEntity::Close() {
  chml::CritScope crit(&crit_);
  closed_ = true;
}

bool TelnetEntity::IsClose() {
  chml::CritScope crit(&crit_);
  return closed_;
}

}	// namespace vty
