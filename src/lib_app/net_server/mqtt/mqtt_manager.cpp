
/**
 * @Description: mqtt
 * @Author:      zhz
 * @Version:     1
 */

#include "mqtt_manager.h"
#include <thread>
#include "eventservice/log/log_client.h"
#include <iostream>

namespace net {

MqttManager::MqttManager(NetServer *net_server_ptr)
  : BaseModule(net_server_ptr),m_needRegisterDevice(false) {

  sprintf(m_topicSubFind, "/VENII/h3/robot/FIND");
  sprintf(m_topicPubAll, "/VENII/h3/robot/GET");

}

MqttManager::~MqttManager() {
  loop_stop();
  mosqpp::lib_cleanup();
}

int MqttManager::Start() {
  init();
  auto func = [&]() {
    runloop();
  };

  std::thread loop_thread(func);
  loop_thread.detach();
  return 0;
}

int MqttManager::Stop() {
  loop_stop();
  mosqpp::lib_cleanup();
  return 0;
} 

bool MqttManager::init() {
  DLOG_INFO(MOD_EB,"*MQTT* init.");
  mosqpp::lib_init();
  if(m_connected)
  {
    disconnect(); //断开连接
  }
  std::string deviceId=MQTT_DEVICE_ID;
  std::string devicePassword=MQTT_DEVICE_PASSWORD;
  std::string deviceName = MQTT_DEVICE_NAME;
  // if(m_needRegisterDevice)
  // {
  //   DLOG_INFO(MOD_EB, "start register mqtt device\n");
  //   if(!registerDevice(deviceId,devicePassword))
  //   {
  //     DLOG_ERROR(MOD_EB, "*MQTT* register device fail.\n");
  //     return false;
  //   }
  // }
  if(!connectMQTT(deviceId.c_str(),deviceName.c_str(),devicePassword.c_str()))
  {
    DLOG_ERROR(MOD_EB, "*MQTT* connect fail.\n");
    return false;
  }
  m_connected = true;
  sleep(2);
  subscribe_to_topic(m_topicSubFind);
  return true;
}

bool MqttManager::getConnectState()
{
    return m_connected;
}
void MqttManager::setConnectState(bool state)
{
    m_connected = state;
}

bool MqttManager::registerDevice(std::string& deviceId,std::string& devicePasswd)
{
  std::string url = "info.deviceRegisterUrl";
  //if(!curl_post_req(url.c_str(),data_str.c_str(),ret_data, 15, true))
  return true;
}

bool MqttManager::connectMQTT(const char* deviceId,const char* deviceName,const char* devicePassword)
{
  // reinitialise(deviceId, true);
  // username_pw_set(deviceName, devicePassword);
  std::string address="192.168.0.38";
  int port=3006;
  int rc = connect_async(address.c_str(), port,60);
  DLOG_INFO(MOD_EB,"*MQTT* connect mqtt.");
  if(rc != MOSQ_ERR_SUCCESS)
  {
    DLOG_ERROR(MOD_EB,"*MQTT* connect fail. rc: %d\n",mosqpp::strerror(rc));
    //MOSQ_ERR_EAI(Lookup error): 1. network is down 2. mqtt address is can not reach
    //MOSQ_ERR_ERRNO(time out):   1. mqtt port is error
    return false;
  }
  DLOG_INFO(MOD_EB,"*MQTT* connect success.");
  loop_start();
  return true;
}

void MqttManager::myPublish(std::string topic, std::string mess)
{
  // int payloadLen = (int)message.length();
  // auto payload = message.c_str();

  // publishNow(topic.c_str(), payloadLen, payload);  
  int ret = publish(NULL, topic.c_str(), mess.size(), mess.c_str(), 1, false);
  if (ret != MOSQ_ERR_SUCCESS){
      std::cout << "Send failed." << std::endl;
  }
  else std::cout << "Send success " << std::endl;
}

void MqttManager::subscribe_to_topic(std::string topic)
{
  int ret = subscribe(NULL, topic.c_str());
  if (ret != MOSQ_ERR_SUCCESS){
      std::cout << "Subcribe failed" << std::endl;
  }
  else std::cout << "Subcribe success " << topic << std::endl;
}
void MqttManager::unsubscribe_from_topic(std::string topic)
{
  int ret = unsubscribe(NULL, topic.c_str());
  if (ret != MOSQ_ERR_SUCCESS){
      std::cout << "Unsubcribe topic " << topic << std::endl;
  }
  else std::cout << "Unsubcribe failed." << std::endl;  
}

// void MqttManager::publishNow(const char* topic,int payloadLen,const char* payload)
// {
//   char *aes_out=NULL;
//   int rc = publish(nullptr,topic,strlen(aes_out),aes_out);
//   if(MOSQ_ERR_SUCCESS == rc)
//   {
//     DLOG_INFO(MOD_EB,"*MQTT* publish success\n");
//   } else {
//     DLOG_ERROR(MOD_EB,"*MQTT* publish fail. error:%d\n" , mosqpp::strerror(rc));
//   }
// }

void MqttManager::on_connect(int rc)
{
    // DLOG_INFO(MOD_EB,"*MQTT* on_connect with code:%d\n",rc);
    // if(MOSQ_ERR_SUCCESS ==rc)
    // {
    //   rc = subscribe(nullptr, m_topicSubFind, 1);
    //   DLOG_INFO(MOD_EB,"*MQTT* subscribe:%d", rc);
    // } else if(MOSQ_ERR_CONN_REFUSED == rc) {
    //     //maybe data remove by server, need register again
    //     m_needRegisterDevice = true;
    // } 
    if ( rc == 0 ) {
     std::cout << "Connected success with server: "<< "this->host" << std::endl;
        // std::cout << "Enter message: ";
        // std::getline(std::cin,mess);
    }
    else{
        std::cout << "Cant connect with server(" << rc << ")" << std::endl;
    }  
}
void MqttManager::on_message(const struct mosquitto_message *message)
{
  std::string payload = std::string(static_cast<char *>(message->payload));
  std::string topic = std::string(message->topic);

  std::cout << "payload: " << payload << std::endl;
  std::cout << "topic: " << topic << std::endl;
  std::cout << "On message( QoS: " << message->mid 
                  << " - Topic: " << std::string(message->topic) << " - Message: "
                  << std::string((char *)message->payload, message->payloadlen) << ")" << std::endl;
 
  // char* payload =(char*)message->payload;
  // DLOG_INFO(MOD_EB,"payload:%s\n",payload);
  // if(strcmp(m_topicSubFind, message->topic) == 0)
  // {
  //   DLOG_INFO(MOD_EB,"topic:%s\n",m_topicSubFind);
  // }
}
//取消连接结果回调通知
void MqttManager::on_disconnect(int rc)
{
  DLOG_INFO(MOD_EB,"*MQTT* on_disconnect with code:%d\n", rc);
  m_connected = false;
}

void MqttManager::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
  DLOG_INFO(MOD_EB,"*MQTT* subscribe succeeded. mid:%d\n",mid);
}

void MqttManager::on_publish(int mid)
{
  DLOG_INFO(MOD_EB,"*MQTT* publish succeeded. mid:%d\n",mid);
}

void MqttManager::runloop()
{
  while(1)
  {
    if(m_connected)
    {
      myPublish(m_topicPubAll, "h3robot from client");
      sleep(10);
    }
  }
}

}