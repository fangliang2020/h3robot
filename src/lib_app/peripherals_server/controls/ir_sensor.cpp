#include "ir_sensor.h"
#include "peripherals_server/main/device_manager.h"
#include "peripherals_server/base/jkey_common.h"

uint16_t    CommParam::left_irrev=0;
uint16_t    CommParam::right_irrev=0;

namespace peripherals{

DP_MSG_HDR(Ir_Sensor_Dev,DP_MSG_DEAL,pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[]={
    {"set_back_com",   &Ir_Sensor_Dev::Set_Back_Com}
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

Ir_Sensor_Dev::Ir_Sensor_Dev(PerServer *per_server_ptr)
    : BaseModule(per_server_ptr),dp_client_(nullptr) {

}
Ir_Sensor_Dev::~Ir_Sensor_Dev() {

}

int Ir_Sensor_Dev::Start() {
   
    rec_fd=-1;
    send_fd =-1;
    DLOG_DEBUG(MOD_EB, "init ir dev\n");
    if(access(IR_SEND_PATH,F_OK)!=0)
    {
        DLOG_ERROR(MOD_EB, "ir dev is not exist!\n");
        return -1;
    }
    send_fd=open(IR_SEND_PATH,O_RDWR);
    if(send_fd<0)
    {
        close(send_fd); 
        DLOG_ERROR(MOD_EB, "open %s dev error!\n",IR_SEND_PATH);
        return -1;
    } 
    if(access(IR_REC_PATH,F_OK)!=0)
    {
        DLOG_ERROR(MOD_EB, "ir dev is not exist!\n");
        return -1;
    }
    rec_fd=open(IR_REC_PATH,O_RDWR);
    if(rec_fd<0)
    {
        close(rec_fd); 
        DLOG_ERROR(MOD_EB, "open %s dev error!\n",IR_REC_PATH);
        return -1;
    } 


    auto func = [&]() {
        runloop();
    };

    std::thread loop_thread(func);
    loop_thread.detach();
    return 0; 
}

int Ir_Sensor_Dev::Stop() {
    DLOG_DEBUG(MOD_EB, "close ir dev\n");
    close(send_fd); 
    close(rec_fd); 
    return 0;
}

int Ir_Sensor_Dev::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
    DLOG_INFO(MOD_EB, "OnDpMessage:%s.", stype.c_str());
    int nret = -1;
    for (uint32 idx = 0; idx < g_dpmsg_table_size; ++idx) {
        if (stype == g_dpmsg_table[idx].type) {
        nret = (this->*g_dpmsg_table[idx].handler) (jreq["body"], jret);
        break;
        }
    }

    return nret;
}

int Ir_Sensor_Dev::Set_Back_Com(Json::Value &jreq, Json::Value &jret)
{   
    std::string com;
    JV_TO_STRING(jreq,"com",com,"");
    int JV_TO_INT(jreq,"clean_mode",clean_mode,0);
    memset(TxBuf,0,2);
    if(0 == strcmp("back_base",com.c_str())) {
        TxBuf[0]=0x5A;
        TxBuf[1]=0xA0;
        write(send_fd,TxBuf,sizeof(TxBuf));
    } 
    else if(0 == strcmp("charging",com.c_str())) {
        TxBuf[0]=0x5A;
        TxBuf[1]=0xA1;
        write(send_fd,TxBuf,sizeof(TxBuf));
    } 
    else if(0 == strcmp("charge_done",com.c_str())) {
        TxBuf[0]=0x5A;
        TxBuf[1]=0xA2;
        write(send_fd,TxBuf,sizeof(TxBuf));
    } 
    else if(0 == strcmp("cleaning",com.c_str())) {
        TxBuf[0]=0x5A;
        TxBuf[1]=0xA3+clean_mode<<4; // clean_mode A3+ 0X10/0X20/0X30  
        write(send_fd,TxBuf,sizeof(TxBuf));
    } 
    else if(0 == strcmp("clean_done",com.c_str())) {
        TxBuf[0]=0x5A;
        TxBuf[1]=0xA4;
        write(send_fd,TxBuf,sizeof(TxBuf));
    } 
    else if(0 == strcmp("stop_back",com.c_str())) {
        TxBuf[0]=0x5A;
        TxBuf[1]=0xA8;
        write(send_fd,TxBuf,sizeof(TxBuf));
    }  
    DLOG_INFO(MOD_EB, "receive com is %s\n",com.c_str());
    return 0;
}

void Ir_Sensor_Dev::runloop(){
    int ret=0;
    while (1) 
    {	
        FD_ZERO(&readfds);
        FD_SET(rec_fd, &readfds);
        /* 构造超时时间 */
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; /* 500ms */
        ret = select(rec_fd + 1, &readfds, NULL, NULL, &timeout);
        switch (ret) 
        {
            case 0: 	/* 超时 */
                /* 用户自定义超时处理 */
                left_irrev=0;
                right_irrev=0;
              //  DLOG_INFO(MOD_EB,"left_irrev=%lx, right_irrev=%lx\r\n",left_irrev,right_irrev); 
                break;
            case -1:	/* 错误 */
                /* 用户自定义错误处理 */
                break;
            default:  /* 可以读取数据 */
                if(FD_ISSET(rec_fd, &readfds))
                {
                    ret = read(rec_fd, &data, sizeof(data));
                    if (ret < 0) {
                        /* 读取错误 */
                        DLOG_ERROR(MOD_EB,"IR receive read error\r\n");
                    }
                    else 
                    {
                        left_irrev=(((uint16_t)data[4])<<8)+data[3];
                        right_irrev=(((uint16_t)data[1])<<8)+data[0];
                    //    DLOG_INFO(MOD_EB,"left_irrev=%lx, right_irrev=%lx\r\n",left_irrev,right_irrev);   				
                    }
                }
                break; 
        }	

    }
}


}