#ifndef _VL6180_SENSOR_H_
#define _VL6180_SENSOR_H_
#include "peripherals_server/base/base_module.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/event/messagehandler.h"
#include "peripherals_server/base/comhead.h"
#include "vl6180x_def.h"
#include "vl6180x_platform.h"
#include "vl6180x_types.h"
#include "vl6180x_cfg.h"
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define USE_I2C2_PORT 0 //不使用i2c节点直接读取
//******************************** IOCTL definitions
#define VL6180_IOCTL_INIT 			_IO('p', 0x01)
#define VL6180_IOCTL_XTALKCALB		_IO('p', 0x02)
#define VL6180_IOCTL_OFFCALB		_IO('p', 0x03)
#define VL6180_IOCTL_STOP			_IO('p', 0x05)
#define VL6180_IOCTL_SETXTALK		_IOW('p', 0x06, unsigned int)
#define VL6180_IOCTL_SETOFFSET		_IOW('p', 0x07, int8_t)
#define VL6180_IOCTL_INIT_ALS 		_IO('p', 0x08)
#define VL6180_IOCTL_STOP_ALS		_IO('p', 0x09)
#define VL6180_IOCTL_GETDATA 		_IOR('p', 0x0a, unsigned long)
#define VL6180_IOCTL_GETDATAS 		_IOR('p', 0x0b, VL6180x_RangeData_t)
#define VL6180_IOCTL_GETLUX 		_IOR('p', 0x0c, unsigned long)

#define EDGE_PWR_OFF system("echo 0 > /sys/devices/platform/pc-switch/pc_switch/switch/do_edge_pwr")
#define EDGE_PWR_ON  system("echo 1 > /sys/devices/platform/pc-switch/pc_switch/switch/do_edge_pwr")
const char VlInputPath[]="/dev/input/event2";
const char vlNormalPath[] ="/dev/stmvl6180_ranging";

namespace peripherals {

enum Vl6180WorkState{
    VL_CLOSE=-1,
    VL_OPEN=0,
    VL_INIT,
    VL_SOFTRESET, //软复位
    VL_HWRESET,//硬复位
    VL_WORK,
};
enum WorkMode{
    MODE_RANGE=0,
    MODE_XTAKCALIB=1,
    MODE_OFFCALIB=2,
    MODE_HELP=3,
    MODE_ALS=4,
    MODE_RANGE_ALS=5,
    SINGLE_DATA=6,
    EVENT_DATA=7,
    IDLE,
};

class Vl6180xManager : public BaseModule,
                       public chml::MessageHandler,
                       public CommParam {
public:
    explicit Vl6180xManager(PerServer *per_server_ptr);
    virtual ~Vl6180xManager();
    int Start();
    int Stop();
    int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
    int OpenVl6180Normal();
    int OpenVl6180Event();
    int InitVl6180Dev();
    int DeInitVl6180Dev();
    int Vl6180_Xtakcalib(Json::Value &jreq, Json::Value &jret);
    int Vl6180_Offcalib(Json::Value &jreq, Json::Value &jret);
    bool Vl6180_Range();
    bool Read_Als();
    bool Read_SingleData();
    bool Read_EventData();
    bool Vl6180_Range_Als();
    int  Vl6180_Run();
    void vl6180_Sample();
public:
    VL6180x_RangeData_t range_datas;
    unsigned long lux_data;
    Vl6180WorkState VlWk_State;//设备的工作状态
    WorkMode SamState;
    bool SampleDone;
    std::mutex m_tex;
private:
    int vl6180_fd;
    int vl6180_fde;
    uint8_t reset_num;
    chml::EventService::Ptr event_service_;
    void OnMessage(chml::Message* msg);
    chml::DpClient::Ptr dp_client_;

#if USE_I2C2_PORT
public:
  constexpr bool is_connected() const { return connected; }
  uint8_t get_range();
private:
      /**
   * @brief Reads an 8-bit value from a 16-bit register location over I2C.
   *
   * @param reg The 16-bit register address.
   * @return The 8-bit register value.
   */
  uint8_t read_register(uint16_t reg);
  
  /**
   * @brief Writes an 8-bit value to a 16-bit register location over I2C.
   *
   * @param reg The 16-bit register address.
   * @param data The 8-bit register value.
   */
  void write_register(uint16_t reg, uint8_t data);
  
  // File descriptor of the I2C device.
  int i2c_fd;
  
  // Whether the device is connected.
  bool connected;
  
  uint8_t range = 0xFF;
  bool is_measuring = false;
#endif
};


}
#endif
