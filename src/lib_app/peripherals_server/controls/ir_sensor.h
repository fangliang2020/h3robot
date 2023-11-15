#ifndef _IR_SENSOR_H_
#define _IR_SENSOR_H_

#include "peripherals_server/base/base_module.h"
#include "peripherals_server/base/comhead.h"

#define IR_REC_PATH					"/dev/ir-receive"
#define IR_SEND_PATH                "/dev/ir-send"

namespace peripherals{

class Ir_Sensor_Dev : public BaseModule,
                    // public chml::MessageHandler,
                    public CommParam {
public:
    explicit Ir_Sensor_Dev(PerServer *per_server_ptr);
    virtual ~Ir_Sensor_Dev();
    int Start();
    int Stop();
    int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
    int Set_Back_Com(Json::Value &jreq, Json::Value &jret);
    void runloop();
private:
    int rec_fd;
    int send_fd;
    fd_set readfds;
	struct timeval timeout; 
    uint8_t TxBuf[2];
    unsigned char data[6];
    chml::DpClient::Ptr dp_client_;
    chml::EventService::Ptr event_service_;
    // void OnMessage(chml::Message* msg);
};
}

#endif