#ifndef _GPIO_MANAGE_H_
#define _GPIO_MANAGE_H_

#include "peripherals_server/base/base_module.h"
#include "peripherals_server/base/comhead.h"
#include "peripherals_server/utils/wheel_pid/pid.h"


#define DEVGPIO_PATH "/dev/h3robot_gpio"
#define H3ROBOT_MAGIC				'G'
#define H3ROBOT_SETOUTPUT			_IOW(H3ROBOT_MAGIC, 0, int)
#define H3ROBOT_SETINPUT			_IOW(H3ROBOT_MAGIC, 1, int)
#define H3ROBOT_SETVALUE			_IOW(H3ROBOT_MAGIC, 2, int)
#define H3ROBOT_GETVALUE			_IOR(H3ROBOT_MAGIC, 3, int)

#define GPIO_ENUM_VALUES \
    X(COLLISION_DET_LA) \
    X(COLLISION_DET_RB) \
    X(COLLISION_DET_RA) \
    X(COLLISION_DET_LB) \
    X(STM_KEY_HOME) \
    X(STM_KEY_DET) \
    X(FILTER_DET) \
    X(LIADR_PI_KEYI1) \
    X(LIADR_PI_KEYI2) \
    X(CLEAN_EPY) \
    X(CLEANTANK_DET) \
    X(SEWAGE_FULL) \
    X(CHG_FULL_STM) \
    X(TB_BRAKE) \
    X(UPDOWN_LM1) \
    X(UPDOWN_LM2) \

#undef X
#define X(x) x,
enum {
    GPIO_ENUM_VALUES
};

namespace peripherals{

const uint8_t gpiosque[]={111,102,153,137,67,77,68,69,113,115,114,142,72,123,8,118};
    // X(COLLISION_DET_LA) \   111
    // X(COLLISION_DET_RB) \   102
    // X(COLLISION_DET_RA) \   153
    // X(COLLISION_DET_LB) \  137
    // X(STM_KEY_HOME) \  67
    // X(STM_KEY_DET) \   77
    // X(FILTER_DET) \    68
    // X(VISOR_DET) \     69
    // X(SEWAGE_DET) \    113
    // X(CLEAN_EPY) \    115
    // X(CLEANTANK_DET) \  114
    // X(SEWAGE_FULL) \   142
    // X(CHG_FULL_STM) \  72
    // X(TB_BRAKE) \   123
    // X(UPDOWN_LM1) \   8
    // X(UPDOWN_LM2) \  118

// class GpioManager;
// typedef std::shared_ptr<GpioManager> GpioManagerPtr;

typedef struct{
    int pin;
    int data;
}gpio_arg;

typedef enum{
    do_edge_pwr=0,  //
    do_lidar_pwr,  //
    do_led1, //
    do_led2, //
    do_led3, //
    do_led4, //
    do_chg_stop, //
    do_cec_mode, //
    do_mcu_pwr, //
    do_sewage_pump,  //
    do_clean_pump, //
   // do_tb_motor, //
    do_tof_lpn, //
    do_scraper_ctl, //
    do_updown_mb, //
    do_updown_ma,
}GPIOOUT_E;

class GpioManager : public BaseModule,
                    public chml::MessageHandler,
                    public CommParam {
public:
    explicit GpioManager(PerServer *per_server_ptr);
    virtual ~GpioManager();
    int Start();
    int Stop();
    int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
    void ReadGpioIn();//读取GPIO电平
    int SetGpioOutput(std::string do_gpio,uint8_t level);
    int robot_state(Json::Value &jreq, Json::Value &jret);
private:
    int gpioin_fd;
    struct pollfd fds;
	fd_set readfds;
	struct timeval timeout;
	unsigned char gpio_rddata[16];
    uint8_t buttons;
    chml::DpClient::Ptr dp_client_;
    chml::EventService::Ptr event_service_;
    void OnMessage(chml::Message* msg);
};
    
}

#endif
