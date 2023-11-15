/**
 * @Description: cmt
 * @Author:      fl
 * @Version:     1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include "cmt_manager.h"
#include "net_server/main/net_server.h"
#include "eventservice/log/log_client.h"
#include "net_server/base/jkey_common.h"

#define MSG_CMT_TIMING       0x500

static uint8_t TxPacket[32]={0};

namespace net {

DP_MSG_HDR(CmtManager,DP_MSG_DEAL,pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[]={
    {"set_back_com",    &CmtManager::Set_Back_Com},//RobotSchedule发送，回仓对接的指令，包括回仓、充电、充电完成、清洗集尘、清洗结束、停止回仓
    {"trigger_data",    &CmtManager::Set_Pair_Com},//peripheral进程按键指令发送，发送配对指令，ioctrl操做，获取SN+8个字节的配对SYNC号。
 //   {"set_ota_com",    &CmtManager::Set_Pair_Com},
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

CmtManager::CmtManager(NetServer *net_server_ptr)
  : BaseModule(net_server_ptr) {
  event_service_ = netptr_->GetEventServicePtr();
  dp_client_ = netptr_->GetDpClientPtr(); 
}

CmtManager::~CmtManager() {
  Stop();
}

int CmtManager::Start() {
  cmt_fd=-1;
  DLOG_DEBUG(MOD_EB, "init cmt2300a\n");
  if(access(DEVCMT_PATH,F_OK)!=0)
  {
      DLOG_ERROR(MOD_EB, "cmt2300a dev is not exist!\n");
      return -1;
  }
  cmt_fd=open(DEVCMT_PATH,O_RDWR | O_NONBLOCK);
  if(cmt_fd<0)
  {
      close(cmt_fd);
      DLOG_ERROR(MOD_EB, "open %s dev error!\n",DEVCMT_PATH);
      return -1;
  }
  return 0;
}

int CmtManager::Stop() {
  DLOG_DEBUG(MOD_EB, "close cmt2300a\n");
  close(cmt_fd);
  return 0;
}

void CmtManager::OnMessage(chml::Message* msg) {
  if (MSG_CMT_TIMING == msg->message_id) {
    RFProcess();
    event_service_->PostDelayed(300, this, MSG_CMT_TIMING);
  }
}

int CmtManager::Set_Back_Com(Json::Value &jreq, Json::Value &jret)
{
  std::string com;
  JV_TO_STRING(jreq,"com",com,"");
  int JV_TO_INT(jreq,"clean_mode",clean_mode,0);
  memset(TxPacket,0,sizeof(TxPacket));
  if(0 == strcmp("back_base",com.c_str())) {
    TxPacket[0]=0x5A;
    TxPacket[1]=0xA0;
    TxPacket[31]=TxPacket[0]+TxPacket[1];
    write(cmt_fd,TxPacket,sizeof(TxPacket));
  } 
  else if(0 == strcmp("charging",com.c_str())) {
    TxPacket[0]=0x5A;
    TxPacket[1]=0xA1;
    TxPacket[31]=TxPacket[0]+TxPacket[1];
    write(cmt_fd,TxPacket,sizeof(TxPacket));
  } 
  else if(0 == strcmp("charge_done",com.c_str())) {
    TxPacket[0]=0x5A;
    TxPacket[1]=0xA2;
    TxPacket[31]=TxPacket[0]+TxPacket[1];
    write(cmt_fd,TxPacket,sizeof(TxPacket));
  } 
  else if(0 == strcmp("cleaning",com.c_str())) {
    TxPacket[0]=0x5A;
    TxPacket[1]=0xA3;
    TxPacket[2]=clean_mode;
    TxPacket[31]=TxPacket[0]+TxPacket[1]+TxPacket[2];
    write(cmt_fd,TxPacket,sizeof(TxPacket));
  } 
  else if(0 == strcmp("clean_done",com.c_str())) {
    TxPacket[0]=0x5A;
    TxPacket[1]=0xA4;
    TxPacket[31]=TxPacket[0]+TxPacket[1];
    write(cmt_fd,TxPacket,sizeof(TxPacket));
  } 
  else if(0 == strcmp("stop_back",com.c_str())) {
    TxPacket[0]=0x5A;
    TxPacket[1]=0xA8;
    TxPacket[31]=TxPacket[0]+TxPacket[1];
    write(cmt_fd,TxPacket,sizeof(TxPacket));
  } 
  DLOG_INFO(MOD_EB, "receive com is %s\n",com.c_str());
  return 0;
}

int CmtManager::Set_Pair_Com(Json::Value &jreq, Json::Value &jret)
{
  uint8_t pair_sn[8];
  int JV_TO_INT(jreq,"buttons",buttons,0);
  if(buttons == 5) //配对433M
  {
    memset(TxPacket,0,sizeof(TxPacket));
    TxPacket[0]=0x5A;
    TxPacket[1]=0xA9;
    memcpy(&TxPacket[2],pair_sn,8);
    for(int i=0;i<10;i++)
    {
      TxPacket[31]+=TxPacket[i];
    }
    write(cmt_fd,TxPacket,sizeof(TxPacket));

    nStatus=RF_STATE_PAIR_INQ;
  }
  return 0;
}

void CmtManager::RFProcess()
{
  switch(nStatus)
  {
    case RF_STATE_IDLE:
        waitcnt=0;
       break;
    case RF_STATE_PAIR_INQ:
        memset(TxPacket,0,sizeof(TxPacket));
        TxPacket[0]=0x5A;
        TxPacket[1]=0xAA;
        TxPacket[31]=TxPacket[0]+TxPacket[1];
        write(cmt_fd,TxPacket,sizeof(TxPacket));
        nStatus=RF_WAIT_ACK;
       break;
    case RF_STATE_UPDATE_INQ:
        memset(TxPacket,0,sizeof(TxPacket));
        TxPacket[0]=0x5A;
        TxPacket[1]=0xA5;
        TxPacket[31]=TxPacket[0]+TxPacket[1];
        write(cmt_fd,TxPacket,sizeof(TxPacket));
        nStatus=RF_WAIT_ACK;
       break;
    case RF_STATE_OTA:
        ioctl(cmt_fd,CMT_COM_OTA,0);
        nStatus=RF_WAIT_ACK;
       break; 
    case RF_WAIT_ACK:
        waitcnt++;
        if(waitcnt>30) {
          waitcnt=0;
          nStatus=RF_STATE_ERROR;
        }
        if(updateflag==1) {
          nStatus=RF_STATE_IDLE;   
        }
        else if(updateflag==2) {
          nStatus=RF_STATE_IDLE;  
        }
       break; 
    case RF_STATE_ERROR:
        /*TODO 增加错误上报*/
        nStatus=RF_STATE_IDLE;
       break;       
  }
}

void CmtManager::ReceiveCommand()
{ 
    int ret;
    int ret_v;
    uint8_t sum;
    FD_ZERO(&readfds);
    FD_SET(cmt_fd, &readfds);
    /* 构造超时时间 */
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000; /* 50ms */
    ret = select(cmt_fd + 1, &readfds, NULL, NULL, &timeout);
    switch (ret) 
    {
      case 0: 	/* 超时 */
          /* 用户自定义超时处理 */
          break;
      case -1:	/* 错误 */
          /* 用户自定义错误处理 */
          break;
      default:  /* 可以读取数据 */
        if(FD_ISSET(cmt_fd, &readfds))
        {
          ret = read(cmt_fd, &cmt_rddata, sizeof(cmt_rddata));
          if (ret < 0) {
              /* 读取错误 */
              DLOG_ERROR(MOD_EB, "cmt2300a read error\r\n");
          }
          else 
          {
            if(cmt_rddata[0]==0x7B){
              for(int i=0;i<31;i++){
                sum+=cmt_rddata[i];
              }
              if(sum==cmt_rddata[31]) {
                ret_v=1;     
              }
              else {
                ret_v=0;
              }
            }
            else ret_v=0;
            if(ret_v) //校验正确
            {
              ParseCommand(cmt_rddata);
            }        
          }
        }
        break; 
    }	
}

void CmtManager::ParseCommand(uint8_t* com) {
  std::string str_req,str_res;
  Json::Value json_req;

  switch(com[1])
  {
    case 0xB0:
    case 0xB1:
    case 0xB3:
    case 0xB4:
    case 0xB7:
      json_req[JKEY_TYPE] = "cmt2300a_cmd";
      json_req[JKEY_BODY]["command"] = com[1];
      json_req[JKEY_BODY]["error"] = 0;
      Json2String(json_req, str_req);
      dp_client_->SendDpMessage("DP_SCHEDULE_REQUEST", 0, str_req.c_str(), str_req.size(), NULL);
    break;

    case 0xB2:
    /*TODO 判断主机是否在基座上，如果在上面才通知主机切换状态*/
    // memset(TxPacket,0,sizeof(TxPacket));
    // TxPacket[0]=0x5A;
    // TxPacket[1]=0xA7;
    // TxPacket[31]=TxPacket[0]+TxPacket[1];
    // write(cmt_fd,TxPacket,sizeof(TxPacket));
    break;

    case 0xBE:
      /*TODO 通知APP错误信息*/
      ErrorStatus = ((uint32)com[2])<<24+((uint32)com[3])<<16+((uint32)com[4])<<8+com[5];
      json_req[JKEY_TYPE] = "cmt2300a_cmd";
      json_req[JKEY_BODY]["command"] = 0;
      json_req[JKEY_BODY]["error"] = ErrorStatus;
      Json2String(json_req, str_req);
      dp_client_->SendDpMessage("DP_SCHEDULE_REQUEST", 0, str_req.c_str(), str_req.size(), NULL);
    break;

    case 0xB5: //升级准备OK
      nStatus=RF_STATE_OTA;
    break;

    case 0xB6: //升级是否成功
      if(com[2]=0){
        updateflag=1;     //不成功     
      }
      else if(com[2]==1) {
        updateflag=2;  //成功
      }
    break;

  }



}
}