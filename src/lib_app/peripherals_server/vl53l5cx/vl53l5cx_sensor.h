#ifndef _VL53L5CX_SENSOR_H_
#define _VL53L5CX_SENSOR_H_

#include "peripherals_server/base/base_module.h"
#include "peripherals_server/base/comhead.h"
#include "uld-driver/inc/vl53l5cx_api.h"
#include "uld-driver/inc/vl53l5cx_plugin_xtalk.h"

#define VL53RIGHT_PATH (char*)"/dev/vl53l5cx_right"
#define VL53MID_PATH   (char*)"/dev/vl53l5cx_mid"
#define VL53LEFT_PATH  (char*)"/dev/vl53l5cx_left"

namespace peripherals{

struct tof_point{
    int x;
    int y;
    uint16_t dis;
};

class Vl53l5cxManager : public BaseModule,
                        public CommParam {
public:  
    explicit Vl53l5cxManager(PerServer *per_server_ptr);
    virtual ~Vl53l5cxManager();
    int Start();
    int Stop();
    int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
    int OpenVl53l5cxDev();
    int InitVl53l5cxDev(VL53L5CX_Configuration *p_dev);
    int DeInitVl53l5cxDev();

    int Vl53l5cx_Xtakcalib(Json::Value &jreq, Json::Value &jret); //0-right 1-mid 2-left
    int Vl53l5cx_VizualizeXtakcalib(VL53L5CX_Configuration *p_dev); //0-right 1-mid 2-left
    int Vl53l5cx_offset(Json::Value &jreq, Json::Value &jret);
    int RebootDev(VL53L5CX_Configuration *p_dev);
    void Vl53l5cx_StartSample();
    void RangingInterrupt(VL53L5CX_Configuration *p_dev);
public:
    VL53L5CX_Configuration 	Vl53_right;
    VL53L5CX_Configuration 	Vl53_mid;
    VL53L5CX_Configuration 	Vl53_left;
    std::vector<tof_point>  tof_vector;
private:
    std::thread *tof_right;
	std::thread *tof_mid;
	std::thread *tof_left;
    static bool need_stop_;
};
    
}
#endif

