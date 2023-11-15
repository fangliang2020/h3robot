/**
 * @Description: mqtt
 * @Author:      zhz
 * @Version:     1
 */

#ifndef SRC_LIB_MQTT_MANAGER_H_
#define SRC_LIB_MQTT_MANAGER_H_

#include "net_server/base/base_module.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/event/messagehandler.h"
#include "mosquitto/include/mosquittopp.h"

#define MQTT_DEVICE_ID "H3rbot1"
#define MQTT_DEVICE_PASSWORD "123456"
#define MQTT_DEVICE_NAME "killers"

#define TOPIC_SUB_DEVICE_FIND_FMT "/VENII/%s/%s/FIND"

#define TOPIC_PUB_ALL_FMT "/VENII/%s/%s/GET"


namespace net {

class MqttManager : public BaseModule,public mosqpp::mosquittopp {
 public:
  explicit MqttManager(NetServer *net_server_ptr);
  virtual ~MqttManager();

  int Start();
  int Stop();
  bool init();
  void event_loop(int timeout_ms =0);
  bool getConnectState();
  void setConnectState(bool state);
  void runloop();
 private:
  bool registerDevice(std::string& deviceId,std::string& devicePasswd);
  bool connectMQTT(const char* deviceId,const char* deviceName,const char* devicePassword);
  void myPublish(std::string topic, std::string mess);
//  void publishNow(const char* topic,int payloadLen=0,const char* payload = nullptr);
  void subscribe_to_topic(std::string topic);
  void unsubscribe_from_topic(std::string topic);
  
  void on_connect(int rc) override; //连接结果回调通知
  void on_disconnect(int rc) override; //取消连接结果回调通知
  void on_message(const struct mosquitto_message *message) override; //收到服务器发送的消息通知
  void on_subscribe(int mid,int qos_count,const int *granted_qos) override; //订阅topic结果回调通知
  void on_publish(int mid) override; //发送结果回调通知
 private:
  bool m_connected;
  bool m_needRegisterDevice; //device param invalid, need register again
  char m_topicPubAll[64];
  char m_topicSubFind[64];
};

}

#endif  // SRC_LIB_MQTT_MANAGER_H_