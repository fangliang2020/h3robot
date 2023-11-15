#ifndef _KVDB_SERVER_H_
#define _KVDB_SERVER_H_



#include "kvdb/kvdb/kvdbclient.h"

namespace cache {
class KvdbServer :
  public chml::MessageHandler,
  public sigslot::has_slots<> {

 public:
  KvdbServer(chml::EventService::Ptr es = chml::EventService::Ptr());
  ~KvdbServer();

  bool Start();

 protected:

  void InitDPClient();
  void OnDpMessage(chml::DpClient::Ptr dp_client, chml::DpMessage::Ptr dp_msg);
  void OnErrorMessage(chml::DpClient::Ptr dp_client, int err);
  virtual void OnMessage(chml::Message* msg);
 
  private:
    chml::DpClient::Ptr dp_client_;
    chml::EventService::Ptr main_service_;
    chml::SocketAddress addr_;
    
    cache::KvdbClient::Ptr kvdb_;
    cache::KvdbClient::Ptr skvdb_;
    cache::KvdbClient::Ptr usr_kvdb_;
  };
} // namespace cache

#endif //_KVDB_SERVER_H_
