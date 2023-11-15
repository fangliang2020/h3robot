#include "eventservice/vty/vty_server.h"
#include "eventservice/vty/vty_client.h"

#include "eventservice/base/basictypes.h"
#include "eventservice/base/sigslot.h"
#include "eventservice/base/criticalsection.h"
#include "eventservice/log/log_client.h"
#include "boost/boost/boost_settings.hpp"

#include "eventservice/log/log_client.h"
#include "eventservice/vty/vty_processor.h"

vty::VtyServer::Ptr GetVtySrvInstance() {
#ifdef ENABLE_VTY
  static vty::VtyServer::Ptr vty_srv_instance_;
  if (!vty_srv_instance_.get()) {
    DLOG_INFO(MOD_EB, "Initing VtySrvInstance.");
    vty_srv_instance_ =
      vty::VtyServer::Ptr(new vty::VtyServer());
    if (vty_srv_instance_.get() != NULL) {
      DLOG_INFO(MOD_EB, "Init VtySrvInstance succeed.");
    } else {
      DLOG_ERROR(MOD_EB, "Init VtySrvInstance failed.");
    }
  }
  return vty_srv_instance_;
#else
  return NULL;
#endif
}

int Vty_Init(uint32 host, int port) {
#ifdef ENABLE_VTY
  vty::VtyServer::Ptr instance = GetVtySrvInstance();
  ASSERT_RETURN_FAILURE(instance.get() == NULL, -1);
  chml::SocketAddress address(host, port);
  ASSERT_RETURN_FAILURE(!instance->Reset(address), -1);
  return 0;
#else
  return -1;
#endif
}

void Vty_Deinit() {
#ifdef ENABLE_VTY
  vty::VtyServer::Ptr instance = GetVtySrvInstance();
  ASSERT_RETURN_VOID(instance.get() == NULL);
  //instance.reset();
  instance->Close();
#endif
}

int Vty_RegistCmd(const char *cmd,
                  VtyCmdHandler cmd_handler,
                  const void *user_data,
                  const char *info) {
#ifdef ENABLE_VTY
  vty::VtyServer::Ptr instance = GetVtySrvInstance();
  ASSERT_RETURN_FAILURE(instance.get() == NULL, -1);
  return instance->RegistCmd(cmd, cmd_handler, user_data, info);
#else
  return -1;
#endif
}

int Vty_DeregistCmd(const char *cmd) {
#ifdef ENABLE_VTY
  vty::VtyServer::Ptr instance = GetVtySrvInstance();
  ASSERT_RETURN_FAILURE(instance.get() == NULL, -1);
  return instance->DeregistCmd(cmd);
#else
  return -1;
#endif
}

int Vty_Print(const void *vty_data, const char *format, ...) {
#ifdef ENABLE_VTY
  ASSERT_RETURN_FAILURE(vty_data == NULL, -1);
  vty::TelnetEntity::Ptr telnet =
    ((vty::TelnetEntity*)(vty_data))->shared_from_this();
  ASSERT_RETURN_FAILURE(telnet.get() == NULL, -1);

  va_list va;
  int rs;
  va_start(va, format);
  rs = telnet->VPrint(format, va);
  va_end(va);

  return rs;
#else
  return -1;
#endif
}