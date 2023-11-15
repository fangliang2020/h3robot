#ifndef __IPC_DAEMON_H__
#define __IPC_DAEMON_H__
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <glib.h>
#include <getopt.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include "app/app/appinterface.h"
#include "daemon_service.h"
#include "db_manager.h"
#include "system_manager.h"
#include "common_ipc.h"

class IpcDaemon : public app::AppInterface,
                  public boost::enable_shared_from_this<IpcDaemon>,
                  public sigslot::has_slots<>{
  public:
    typedef boost::shared_ptr<IpcDaemon> Ptr;
    IpcDaemon();
    virtual ~IpcDaemon();
    
    bool PreInit(chml::EventService::Ptr event_service);

    bool InitApp(chml::EventService::Ptr event_service);

    bool RunAPP(chml::EventService::Ptr event_service);

    void OnExitApp(chml::EventService::Ptr event_service);

    void Ipc_Monitor();

    void u_upgrade();

    int ResetFactory(Json::Value &jreq, Json::Value &jret);

    int Ota_Upate(Json::Value &jreq, Json::Value &jret);

    void OpenWdt();
  private:
    void OnDpMessage(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg);
 
  private:
    std::thread *m_ipcTh;
    std::thread *m_otaTh;
    chml::DpClient::Ptr dp_client;
    int wdt_fd;
    int ret;
};

#endif

