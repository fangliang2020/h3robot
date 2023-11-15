/**
 * @Description: peripherals server
 * @Author:      fl
 * @Version:     1
 */
#ifndef SRC_LIB_PERIPHERALS_SERVER_H_
#define SRC_LIB_PERIPHERALS_SERVER_H_
#include "core_include.h"
#include "app/app/appinterface.h"
#include "../base/base_module.h"
#include "peripherals_server/base/comhead.h"

namespace peripherals{

class PerServer : public app::AppInterface,
                  public boost::enable_shared_from_this<PerServer>,
                  public sigslot::has_slots<>,
                  public chml::MessageHandler,
                  public CommParam {
  public:
    typedef boost::shared_ptr<PerServer> Ptr;
    typedef std::vector<BaseModule*> VecBaseModule;

    PerServer();
    virtual ~PerServer();
    
    bool PreInit(chml::EventService::Ptr event_service);

    bool InitApp(chml::EventService::Ptr event_service);

    bool RunAPP(chml::EventService::Ptr event_service);

    void OnExitApp(chml::EventService::Ptr event_service);

    chml::DpClient::Ptr GetDpClientPtr();
    chml::EventService::Ptr GetEventServicePtr();

    void VoicePlaySelf(const char *tts);
  private:
    void OnDpMessage(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg);
    bool CreateBaseNode();
    bool InitBaseNode();
    void OnMessage(chml::Message* msg);
    void SensorBroadcast();
  private:
    VecBaseModule base_node_;
    chml::DpClient::Ptr dp_client_rev;
    chml::DpClient::Ptr dp_client_send;
    chml::EventService::Ptr MyEvent_Service_;
    chml::EventService::Ptr Event_Sensor_;
};

}


#endif


