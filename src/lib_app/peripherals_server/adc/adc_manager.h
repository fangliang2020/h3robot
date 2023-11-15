#ifndef  ADC_MANAGER_H
#define  ADC_MANAGER_H

#include "peripherals_server/base/base_module.h"
#include "peripherals_server/base/comhead.h"
#include "eventservice/event/thread.h"
#define  DEVADC_PATH "/dev/ir-adc"
#define ADCK  1.8f/1023.0f
#define TIME_MAXCNT 10

#define MY_ENUM_VALUES \
    X(FAN_ISENSE) \
    X(UPDOWN_MOTOR_ISENSE) \
    X(SIDEA_SWEEP_ISENSE) \
    X(VBAT_ADC) \
    X(ADAPTOR_INSERT) \
    X(R_WHEEL_ISENSE) \
    X(L_WHEEL_ISENSE) \
    X(SIDEB_SWEEP_ISENSE) \
    X(CLEAN_PUMP_ADC) \
    X(MID_SWEEP_ISENSE) \
    X(SEWAGE_PUMP_ADC) \
    X(SCRAPER_MOTOR_ADC) \
    X(GROUND_DET_RB_ADC) \
    X(GROUND_DET_RF_ADC) \
    X(GROUND_DET_LB_ADC) \
    X(GROUND_DET_LF_ADC) \

#undef X
#define X(x) x,
enum {
    MY_ENUM_VALUES
};

namespace peripherals{


class AdcManager : public BaseModule,
                   public chml::MessageHandler,
                   public CommParam {
public:
  explicit AdcManager(PerServer *per_server_ptr);
  virtual ~AdcManager();
  int Start();
  int Stop();
  int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret);
  int Gnddet_Calibrate(Json::Value &jreq, Json::Value &jret);
  void Adc_Sample();

  int sma_filter(const int &raw,std::queue<int> &_queue,int &_last_flt_value) {
    _queue.push(raw);
    if(_queue.size()>10) {  
      _last_flt_value+=((raw - _queue.front())/10);
      _queue.pop();
    } else {
      _last_flt_value = (_last_flt_value*(_queue.size()-1)+raw) / _queue.size();
    }
    return _last_flt_value;
  }
  void CatValue(int *data);
  void ConversionToStandards();	//准换为标准单位		
	void ProtectCheck();				//调试时可以先屏蔽
  void SensorBroadcast(); 
private:
  int adc_fd;
  struct pollfd fds;
	fd_set readfds;
	struct timeval timeout;
  int readdata[16];
  void OnMessage(chml::Message* msg);
  chml::EventService::Ptr event_service_;
  std::map<std::string,uint8_t> SampleData;
  std::vector<std::queue<int>> _filter;
  std::vector<int> _last_flt;
  chml::DpClient::Ptr dp_client_;
};



}

#endif // ! 
