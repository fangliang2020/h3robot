#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

#include "peripherals_server/base/base_module.h"
#include "peripherals_server/base/comhead.h"

extern std::mutex tex;
extern std::condition_variable cv;
extern bool ready;

#define DEVIMU_PATH "/dev/icm42670"
/* default FSR for gyro
 * note: 4000dps is supported only by icm42686.
 * 0:250dps, 1:500dps, 2:1000dps, 3:2000dps, 4:4000dps
 */
#define MPU_INIT_GYRO_SCALE      3
/* default FSR for accel
 * note: 32g is supported by only by icm42686. 2g is not supported by icm42686.
 * 0:2g, 1:4g, 2:8g, 3:16g, 4:32g
 */
#define MPU_INIT_ACCEL_SCALE     0  //调整accel分辨率

namespace peripherals{

typedef std::chrono::steady_clock               sc;
typedef std::chrono::steady_clock::time_point   tp;
typedef std::chrono::duration<double,std::micro> dd;

typedef struct 
{
    float LastP;//ÉÏ´Î¹ÀËãÐ­·½²î ³õÊ¼»¯ÖµÎª0.02
    float Now_P;//µ±Ç°¹ÀËãÐ­·½²î ³õÊ¼»¯ÖµÎª0
    float out;//¿¨¶ûÂüÂË²¨Æ÷Êä³ö ³õÊ¼»¯ÖµÎª0
    float Kg;//¿¨¶ûÂüÔöÒæ ³õÊ¼»¯ÖµÎª0
    float Q;//¹ý³ÌÔëÉùÐ­·½²î ³õÊ¼»¯ÖµÎª0.001
    float R;//¹Û²âÔëÉùÐ­·½²î ³õÊ¼»¯ÖµÎª0.543
}KFP;//Kalman Filter parameter

class ImuManager : public BaseModule,
                   public chml::MessageHandler,
                   public CommParam {
public:
    explicit ImuManager(PerServer *per_server_ptr);
    virtual ~ImuManager();
    int Start();
    int Stop();
    int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
    void Imu_Run();
    int Imu_Calibrate(Json::Value &jreq, Json::Value &jret);
    void posture_cal(double ts_diff); 
    void SensorBroadcast();
public: 

private:
    static int imu_fd;
    struct pollfd fds;
	fd_set readfds;
    struct timeval timeout;
    static int imu_calibrate_flag;
    chml::EventService::Ptr event_service_;
    void OnMessage(chml::Message* msg);
    chml::DpClient::Ptr dp_client_;
};

}
#endif
