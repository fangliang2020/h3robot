/**
 * @Description: cmt2300a
 * @Author:      fl
 * @Version:     1
 */

#ifndef SRC_LIB_CMT_MANAGER_H_
#define SRC_LIB_CMT_MANAGER_H_

#include "net_server/base/base_module.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/event/messagehandler.h"

#define DEVCMT_PATH "/dev/cmt2300a"
#define CMT_COM_MAGIC 			'M'
#define CMT_COM_OTA       _IOW(CMT_COM_MAGIC, 0, int)
#define CMT_COM_PAIR		  _IOR(CMT_COM_MAGIC, 1, int)
#define CMT_COM_MAXNR				1


namespace net {

typedef enum {
    RF_STATE_IDLE = 0,
    RF_STATE_PAIR_INQ, //询问配对
    RF_STATE_UPDATE_INQ, //询问是否升级
    RF_STATE_OTA, //OTA升级
    RF_STATE_UPDATE_STATUS,//升级是否成功
    RF_WAIT_ACK,
    RF_STATE_ERROR,
} EnumRFStatus;

class CmtManager : public BaseModule,
                   public chml::MessageHandler {
 public:
  explicit CmtManager(NetServer *net_server_ptr);
  virtual ~CmtManager();

  int Start();
  int Stop();
  int Set_Back_Com(Json::Value &jreq, Json::Value &jret);
  int Set_Pair_Com(Json::Value &jreq, Json::Value &jret);
  void RFProcess();
  void ReceiveCommand();//接收指令
  void ParseCommand(uint8_t* com);
 private:
  void OnMessage(chml::Message* msg);
  int cmt_fd;
	fd_set readfds;
  struct timeval timeout;
 private:
  chml::EventService::Ptr event_service_;
  chml::DpClient::Ptr dp_client_;
  EnumRFStatus nStatus; 
  uint16_t waitcnt;
  uint8_t cmt_rddata[32];
  uint32_t ErrorStatus;
  uint8_t updateflag;
};

}

#endif  // SRC_LIB_CMT_MANAGER_H_