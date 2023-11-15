/**
 * @Description: db server
 * @Author:      fl
 * @Version:     1
 */
#ifndef SRC_LIB_DB_SERVER_H_
#define SRC_LIB_DB_SERVER_H_
#include "core_include.h"
#include "app/app/appinterface.h"
#include "db_server/base/base_module.h"
#include "db_server/parseyaml/yamlparse.h"

#define YAMLPATH (char*)"/oem/cfg/config.yaml"
#define DBPATH  (char*)"/oem/cfg/sysConfig.db"


namespace db{

class DbServer : public app::AppInterface,
                public boost::enable_shared_from_this<DbServer>,
                public sigslot::has_slots<> {
  public:
    typedef boost::shared_ptr<DbServer> Ptr;
    typedef std::vector<BaseModule*> VecBaseModule;

    DbServer();
    virtual ~DbServer();
    
    bool PreInit(chml::EventService::Ptr event_service);

    bool InitApp(chml::EventService::Ptr event_service);

    bool RunAPP(chml::EventService::Ptr event_service);

    void OnExitApp(chml::EventService::Ptr event_service);

    chml::DpClient::Ptr GetDpClientPtr();
    chml::EventService::Ptr GetEventServicePtr();
  private:
    void OnDpMessage(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg);
    bool CreateBaseNode();
    bool InitBaseNode();
    void SensorBroadcast();
  private:
    VecBaseModule base_node_;
    chml::DpClient::Ptr dp_client_;
    chml::EventService::Ptr Event_Service_;
    YamlConfig *YamlConf;
    bool flag_dbexit;
};

}
#endif
