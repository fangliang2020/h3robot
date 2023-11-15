#include "gpio_manager.h"
#include "peripherals_server/main/device_manager.h"
#include "peripherals_server/base/jkey_common.h"
#define MSG_GPIO_TIMING 0x105
// 重新定义宏函数并获取新的展开形式，以获取枚举值对应的字符串；
const char *enum2str(int n) {
#undef X
#define X(x) case (x): { return #x; }
#define MAKE_ENUM_CASES \
    GPIO_ENUM_VALUES \
    default: { return "unknown enum string."; }
    switch (n) {
        MAKE_ENUM_CASES
    }
}
std::map<std::string,uint8_t> CommParam::Gpio_PinD {
    {"COLLISION_DET_LA",0},{"COLLISION_DET_RB",0},
    {"COLLISION_DET_RA",0},{"COLLISION_DET_LB",0},
    {"STM_KEY_HOME",0},{"STM_KEY_DET",0},
    {"FILTER_DET",0},{"LIADR_PI_KEYI1",0},
    {"LIADR_PI_KEYI2",0},{"CLEAN_EPY",0},
    {"CLEANTANK_DET",0},{"SEWAGE_FULL",0},
    {"CHG_FULL_STM",0},{"TB_BRAKE",0},
    {"UPDOWN_LM1",0},{"UPDOWN_LM2",0},
};
uint8_t CommParam::abnormal=0;

namespace peripherals{

DP_MSG_HDR(GpioManager,DP_MSG_DEAL,pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[]={
    {"robot_device_state",   &GpioManager::robot_state}
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

GpioManager::GpioManager(PerServer *per_server_ptr)
    : BaseModule(per_server_ptr),dp_client_(nullptr) {
    dp_client_ = Perptr_->GetDpClientPtr();
    buttons=0;
}
GpioManager::~GpioManager() {

}

int GpioManager::Start() {
   
    gpioin_fd=-1;
    DLOG_DEBUG(MOD_EB, "init gpio dev\n");
    if(access(DEVGPIO_PATH,F_OK)!=0)
    {
        DLOG_ERROR(MOD_EB, "gpioin dev is not exist!\n");
        return -1;
    }
    gpioin_fd=open(DEVGPIO_PATH,O_RDWR);
    if(gpioin_fd<0)
    {
        close(gpioin_fd); 
        DLOG_ERROR(MOD_EB, "open %s dev error!\n",DEVGPIO_PATH);
        return -1;
    } 
    event_service_ = chml::EventService::CreateEventService(NULL, "gpio_readin");
    ASSERT_RETURN_FAILURE(!event_service_, -1);
    event_service_->PostDelayed(100, this, MSG_GPIO_TIMING);
    return 0; 
}

int GpioManager::Stop() {
    DLOG_DEBUG(MOD_EB, "close gpioin dev\n");
    close(gpioin_fd); 
    return 0;
}

int GpioManager::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
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

int GpioManager::robot_state(Json::Value &jreq, Json::Value &jret) {
    int JV_TO_INT(jreq, "device_state", dev_state, 1);
     DLOG_INFO(MOD_EB, "robot state:%s\n.", jreq.toStyledString().c_str());
    if(dev_state==17) {
        SetGpioOutput("do_mcu_pwr",0);
        DLOG_INFO(MOD_EB, "shut down\n");
    }
    return -1; //return -1 继续往下走
}
int GpioManager::SetGpioOutput(std::string do_gpio,uint8_t level)
{
    char echostr[80];
    sprintf(echostr,"echo %d >  /sys/devices/platform/pc-switch/pc_switch/switch/%s",level,do_gpio.c_str());
    printf(echostr);
    system(echostr);
    return 0;
}

/* TODO LED control*/

void GpioManager::OnMessage(chml::Message* msg) {
  if (MSG_GPIO_TIMING == msg->message_id) {
    ReadGpioIn();
    event_service_->Post(this, MSG_GPIO_TIMING);
  }
}

void GpioManager::ReadGpioIn()
{ 
    int ret;
    FD_ZERO(&readfds);
    FD_SET(gpioin_fd, &readfds);
    Gpio_PinD.find("STM_KEY_HOME")->second=0;
    Gpio_PinD.find("STM_KEY_DET")->second=0;
    /* 构造超时时间 */
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000; /* 50ms */
    ret = select(gpioin_fd + 1, &readfds, NULL, NULL, &timeout);
    switch (ret) 
    {
        case 0: 	/* 超时 */
            /* 用户自定义超时处理 */
            break;
        case -1:	/* 错误 */
            /* 用户自定义错误处理 */
            break;
        default:  /* 可以读取数据 */
            if(FD_ISSET(gpioin_fd, &readfds))
            {
                ret = read(gpioin_fd, &gpio_rddata, sizeof(gpio_rddata));
                if (ret < 0) {
                    /* 读取错误 */
                    DLOG_ERROR(MOD_EB, "gpio_manager read error\r\n");
                }
                else 
                {
                    buttons=0;
                    for(int i=0;i<16;i++)
                    {
                        Gpio_PinD.find(enum2str(i))->second=gpio_rddata[i];                      
                        //如果是 按键触发信号，则直接发送，如果是其他传感器信号则统一发送
                        DLOG_INFO(MOD_EB,"key=%s, value=%d\r\n",enum2str(i), Gpio_PinD.at(enum2str(i)));
                    }
                    //有按键按下
                    if((Gpio_PinD.find("STM_KEY_HOME")->second)||(Gpio_PinD.find("STM_KEY_DET")->second)) {
                        std::string str_req,str_res;
                        Json::Value json_req;
                        //双击按钮
                        if((Gpio_PinD.find("STM_KEY_HOME")->second==2)&&(Gpio_PinD.find("STM_KEY_DET")->second==2)) {
                            buttons=4;//双击配网键触发 
                        }   
                        else if((Gpio_PinD.find("STM_KEY_HOME")->second==1)||(Gpio_PinD.find("STM_KEY_HOME")->second==2))  {
                            buttons=3;//回仓暂停键短按(切换回仓和暂停状态)
                        }
                        else if(Gpio_PinD.find("STM_KEY_DET")->second==1)  {
                            buttons=1; //开始暂停键短按(切换清扫和暂停状态)
                        }
                        else if(Gpio_PinD.find("STM_KEY_DET")->second==2)  {
                            buttons=2; //开始暂停键长按(关机)
                        }
                        json_req[JKEY_TYPE] = "trigger_data";
                        json_req[JKEY_BODY]["buttons"] = buttons;
                        json_req[JKEY_BODY]["voice"] = 0;
                        Json2String(json_req, str_req);
                        dp_client_->SendDpMessage("DP_PERIPHERALS_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
                    }
                }
            }
            break; 
    }	
}
   
}
