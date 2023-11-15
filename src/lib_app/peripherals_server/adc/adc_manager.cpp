#include "adc_manager.h"
#include "peripherals_server/main/device_manager.h"
#include "net_server/base/jkey_common.h"

#define MSG_ADC_TIMING 0x104

POWER_ADC   CommParam::AdaptIn_Prot;
POWER_ADC   CommParam::VBAT_Prot;
Motol_Ctrl  CommParam::Motorfan;
Motol_Ctrl  CommParam::Motormid;
Motol_Ctrl  CommParam::MotorsideA;
Motol_Ctrl  CommParam::MotorsideB;
Motol_Ctrl  CommParam::MotorMop;
Motol_Ctrl  CommParam::UpdownMotor;
Motol_Ctrl  CommParam::RightWheel;
Motol_Ctrl  CommParam::LeftWheel;
Motol_Ctrl  CommParam::MotorCleanPump;
Motol_Ctrl  CommParam::MotorSewagePump;

GROUD_DET  CommParam::GND_Sensor_RB;
GROUD_DET  CommParam::GND_Sensor_RF;
GROUD_DET  CommParam::GND_Sensor_LB;
GROUD_DET  CommParam::GND_Sensor_LF;


// 重新定义宏函数并获取新的展开形式，以获取枚举值对应的字符串；
const char *enum2string(int n) {
#undef X
#define X(x) case (x): { return #x; }
#define MAKE_ENUM_CASES \
    MY_ENUM_VALUES \
    default: { return "unknown enum string."; }
    switch (n) {
        MAKE_ENUM_CASES
    }
}

namespace peripherals{
DP_MSG_HDR(AdcManager,DP_MSG_DEAL,pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[]={
    {"gnddet_calibrate",   &AdcManager::Gnddet_Calibrate},
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

AdcManager::AdcManager(PerServer *per_server_ptr)
    : BaseModule(per_server_ptr),dp_client_(nullptr) {
  _filter.reserve(12);
  _last_flt.reserve(12);
	dp_client_ = Perptr_->GetDpClientPtr();
}

AdcManager::~AdcManager() {

}

int AdcManager::Start() {
  adc_fd=-1;
  DLOG_DEBUG(MOD_EB, "init adc dev\n");
  if(access(DEVADC_PATH,R_OK)!=0) //检查设备节点是否存在
  {
    return -1;
  }
  adc_fd=open(DEVADC_PATH, O_RDWR | O_NONBLOCK);
  if(adc_fd<0)
	{
    close(adc_fd);
    DLOG_ERROR(MOD_EB, "open %s dev error!\n",DEVADC_PATH);
		return -1;
	} 
  event_service_ = chml::EventService::CreateEventService(NULL, "adc_sample");
  ASSERT_RETURN_FAILURE(!event_service_, -1);
  event_service_->PostDelayed(100, this, MSG_ADC_TIMING);
  return 0; 
}

int AdcManager::Stop() {
    DLOG_DEBUG(MOD_EB, "close adc dev\n");
    close(adc_fd); 
    return 0; 
}

int AdcManager::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
    DLOG_DEBUG(MOD_EB, "OnDpMessage:%s.", stype.c_str());
    int nret = -1;
    for (uint32 idx = 0; idx < g_dpmsg_table_size; ++idx) {
        if (stype == g_dpmsg_table[idx].type) {
        nret = (this->*g_dpmsg_table[idx].handler) (jreq["body"], jret);
        break;
        }
    }

    return nret;
}

int AdcManager::Gnddet_Calibrate(Json::Value &jreq, Json::Value &jret) {

  
  return -1;
}

void AdcManager::OnMessage(chml::Message* msg) {
  if (MSG_ADC_TIMING == msg->message_id) {
    Adc_Sample();
    event_service_->Post(this, MSG_ADC_TIMING);
  }
}

void AdcManager::Adc_Sample() {
  int ret;
  FD_ZERO(&readfds);
  FD_SET(adc_fd, &readfds);
  /* 构造超时时间 */
  timeout.tv_sec = 0;
  timeout.tv_usec = 50000; /* 500ms */
  ret = select(adc_fd + 1, &readfds, NULL, NULL, &timeout);
  switch (ret) 
  {
    case 0: 	/* 超时 */
      /* 用户自定义超时处理 */
      break;
    case -1:	/* 错误 */
      /* 用户自定义错误处理 */
      break;
    default:  /* 可以读取数据 */
      if(FD_ISSET(adc_fd, &readfds))
      {
        ret = read(adc_fd, &readdata, sizeof(readdata));
        if (ret < 0) {
          /* 读取错误 */
          printf("read error\r\n");
        }
        else 
        {
        //   SampleData.clear();
        //   for(int i=0;i<16;i++)
        //   {
        //       if(i < 12) {
        //         SampleData[enum2string(i)]= sma_filter(readdata[i],_filter[i],_last_flt[i]);
        //       }
        //       else {
        //         SampleData[enum2string(i)]=readdata[i];//地检数据不用滤波  
        //       }      
        //  //   printf("key=%s, value=%d\r\n",enum2string(i),SampleData.at(enum2string(i)));   
        //   }	
          CatValue(readdata);
          ConversionToStandards();		
        }
      }
      break; 

  }	

}

void AdcManager::CatValue(int *data) {
  // Motorfan.M_Prot.readValue = SampleData.at("FAN_ISENSE");
	// UpdownMotor.M_Prot.readValue = SampleData.at("UPDOWN_MOTOR_ISENSE");
  // MotorsideA.M_Prot.readValue = SampleData.at("SIDEA_SWEEP_ISENSE");
	// VBAT_Prot.readValue = SampleData.at("VBAT_ADC");
	// AdaptIn_Prot.readValue = SampleData.at("ADAPTOR_INSERT");
	// RightWheel.M_Prot.readValue = SampleData.at("R_WHEEL_ISENSE");
	// LeftWheel.M_Prot.readValue = SampleData.at("L_WHEEL_ISENSE");
	// MotorsideB.M_Prot.readValue = SampleData.at("SIDEB_SWEEP_ISENSE");
	// MotorCleanPump.M_Prot.readValue = SampleData.at("CLEAN_PUMP_ADC");
	// Motormid.M_Prot.readValue = SampleData.at("MID_SWEEP_ISENSE");
	// MotorSewagePump.M_Prot.readValue = SampleData.at("SEWAGE_PUMP_ADC");
	// MotorMop.M_Prot.readValue = SampleData.at("SCRAPER_MOTOR_ADC");
	// GND_Sensor_RB.readVal = SampleData.at("GROUND_DET_RB_ADC");   
	// GND_Sensor_RF.readVal = SampleData.at("GROUND_DET_RF_ADC");   
	// GND_Sensor_LB.readVal = SampleData.at("GROUND_DET_LB_ADC");   
	// GND_Sensor_LF.readVal = SampleData.at("GROUND_DET_LF_ADC");  
	Motorfan.M_Prot.readValue =data[0];
	UpdownMotor.M_Prot.readValue = data[1];
  MotorsideB.M_Prot.readValue = data[2]; //B是右边
	VBAT_Prot.readValue = data[3];
	RightWheel.M_Prot.readValue = data[4];
	AdaptIn_Prot.readValue  = data[5];
	LeftWheel.M_Prot.readValue = data[6];
	MotorsideA.M_Prot.readValue = data[7]; //A是左边
	MotorCleanPump.M_Prot.readValue = data[8];
	Motormid.M_Prot.readValue = data[9];
	MotorSewagePump.M_Prot.readValue = data[10];
	MotorMop.M_Prot.readValue = data[11];
	GND_Sensor_RB.readVal = data[12];  
	GND_Sensor_RF.readVal = data[13];  
	GND_Sensor_LB.readVal = data[14];  
	GND_Sensor_LF.readVal = data[15];
	// DLOG_INFO(MOD_EB, "data[0]=%d,data[1]=%d,data[2]=%d,data[3]=%d,data[4]=%d,data[5]=%d,data[6]=%d,data[7]=%d,data[8]=%d,data[9]=%d,\
	// 									 data[10]=%d,data[11]=%d,GND_Sensor_RB.readVal=%d,GND_Sensor_RF.readVal=%d,GND_Sensor_LB.readVal=%d,GND_Sensor_LF.readVal=%d,\n",\
	// 									 data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],GND_Sensor_RB.readVal,GND_Sensor_RF.readVal,GND_Sensor_LB.readVal,GND_Sensor_LF.readVal);
}

void AdcManager::ConversionToStandards() {
  Motorfan.M_Prot.currentValue =(float)(Motorfan.M_Prot.readValue*0.005652423f); //(float)(Motorfan.M_Prot.readValue*ADCK)*5.81f;
	UpdownMotor.M_Prot.currentValue = (float)(UpdownMotor.M_Prot.readValue*ADCK)*17.83f;
  MotorsideA.M_Prot.currentValue = (float)(MotorsideA.M_Prot.readValue*0.000938631f); //(float)(MotorsideA.M_Prot.readValue*ADCK)*17.83f;
	VBAT_Prot.currentValue = (float)(VBAT_Prot.readValue*ADCK)/0.545f/0.1275f; //已调整
	//printf("vbat is %3.2f\n",VBAT_Prot.currentValue);
	AdaptIn_Prot.currentValue =(float)(AdaptIn_Prot.readValue*ADCK)/0.545f/0.131f;
	RightWheel.M_Prot.currentValue =(float)(RightWheel.M_Prot.readValue*0.003264162f);//(float)(RightWheel.M_Prot.readValue*ADCK)*17.83f;
	LeftWheel.M_Prot.currentValue =(float)(LeftWheel.M_Prot.readValue*0.003084689f);  //(float)(LeftWheel.M_Prot.readValue*ADCK)*17.83f;
	MotorsideB.M_Prot.currentValue =(float)(MotorsideB.M_Prot.readValue*0.001091046f);// (float)(MotorsideB.M_Prot.readValue*ADCK)*17.83f;
	MotorCleanPump.M_Prot.currentValue = (float)(MotorCleanPump.M_Prot.readValue*ADCK)*17.83f;
	Motormid.M_Prot.currentValue = (float)(Motormid.M_Prot.readValue*0.002437184f); //(float)(Motormid.M_Prot.readValue*ADCK)*10.14f;
	MotorSewagePump.M_Prot.currentValue = (float)(MotorSewagePump.M_Prot.readValue*ADCK)*17.83f;
	MotorMop.M_Prot.currentValue = (float)(MotorMop.M_Prot.readValue*ADCK)*10.14f;
	// DLOG_INFO(MOD_EB,"VBAT_Prot=%5.3f,AdaptIn_Prot=%5.3f\n",VBAT_Prot.currentValue,AdaptIn_Prot.currentValue);
	// DLOG_INFO(MOD_EB, "Motorfan=%5.3f,UpdownMotor=%5.3f,MotorsideA=%5.3f,VBAT_Prot=%5.3f,AdaptIn_Prot=%5.3f,RightWheel=%5.3f,LeftWheel=%5.3f,MotorsideB=%5.3f,MotorCleanPump=%5.3f,\
	// 								   Motormid=%5.3f,MotorSewagePump=%5.3f,MotorMop=%5.3f\n",\
	// 									 Motorfan.M_Prot.currentValue,UpdownMotor.M_Prot.currentValue,MotorsideA.M_Prot.currentValue,VBAT_Prot.currentValue,AdaptIn_Prot.currentValue,RightWheel.M_Prot.currentValue,\
	// 									 LeftWheel.M_Prot.currentValue,MotorsideB.M_Prot.currentValue,MotorCleanPump.M_Prot.currentValue,Motormid.M_Prot.currentValue,MotorSewagePump.M_Prot.currentValue,MotorMop.M_Prot.currentValue);	
}

//写到各自的保护程序中
void AdcManager::ProtectCheck() {
 	/**************电源适配保护检测**************/
	if(AdaptIn_Prot.currentValue>=AdaptIn_Prot.OverValue){
		AdaptIn_Prot.overcount++;
		if(AdaptIn_Prot.overcount>=TIME_MAXCNT) 
		{
			AdaptIn_Prot.status=1; //过压状态
			AdaptIn_Prot.undercount=0;	
		}
	}
	else if((AdaptIn_Prot.currentValue>16.5f)&&(AdaptIn_Prot.currentValue<AdaptIn_Prot.UnderValue)){
		AdaptIn_Prot.undercount++;
		if(AdaptIn_Prot.undercount>=TIME_MAXCNT) 
		{
			AdaptIn_Prot.status=2; //欠压状态
			AdaptIn_Prot.overcount=0;
		}
	}
	else if((AdaptIn_Prot.currentValue>AdaptIn_Prot.UnderValue)&&(AdaptIn_Prot.currentValue<AdaptIn_Prot.OverValue))
	{
	    if(AdaptIn_Prot.overcount>=2*TIME_MAXCNT)
			{
					AdaptIn_Prot.undercount=0;
					// if(AdaptIn_Prot.status!=4)
					// {
					// 		AdaptIn_Prot.status=4;//开始切换状态
					// 		if((gSystem_info.new_workstatus==Cleaning)||(gSystem_info.new_workstatus==Clean_Done)||(gSystem_info.new_workstatus==CleanBack)\
					// 			||(gSystem_info.new_workstatus==Charging)||(gSystem_info.new_workstatus==Charge_Done))
					// 		{
					// 				__nop();
					// 		}
					// 		else
					// 		{
					// 				if(gSystemMonitor.MonitorMode==Normal)
					// 				{
					// 						Change_WorkStatus(Charging);
					// 				}
					// 		}
					// }
					// else
					// {
					// 	  if(gSystem_info.new_workstatus==Standby)
					// 		{
					// 			Change_WorkStatus(Charging);
					// 		}
					// }
			}
			else if((AdaptIn_Prot.overcount>=TIME_MAXCNT)&&(AdaptIn_Prot.overcount<2*TIME_MAXCNT)) {
					AdaptIn_Prot.status=3;
					AdaptIn_Prot.overcount++;
			}
			else {
					AdaptIn_Prot.overcount++;
			}
	}
	else{
		// gSystem_info.Error_Category&=~0x0001;
		// gSystem_info.Error_Category&=~(0x0002);
		AdaptIn_Prot.status=0;
		AdaptIn_Prot.overcount=0;
		AdaptIn_Prot.undercount=0;
//         if(gSystem_info.new_workstatus==Clean_Done)
//         {
//             __nop();
//         }
// 				else if((gSystem_info.new_workstatus==Charging)||(gSystem_info.new_workstatus==Charge_Done))
// //        else if((gSystem_info.new_workstatus==Charging)||(gSystem_info.new_workstatus==Cleaning))
//         {

//         }
	} 
	/**************电池保护检测**************/
	if(VBAT_Prot.currentValue>=VBAT_Prot.OverValue){
		VBAT_Prot.overcount++;
		if(VBAT_Prot.overcount>=TIME_MAXCNT) 
		{
			VBAT_Prot.status=1; //过压状态
            // gSystem_info.Perbat=100;
		//	STM_CHG_STOP_ON;
			VBAT_Prot.undercount=0;	
			// gSystem_info.Error_Category|=1<<2;
		}

	}
	else if(VBAT_Prot.currentValue<VBAT_Prot.UnderValue){
		VBAT_Prot.undercount++;
		if(VBAT_Prot.undercount>=TIME_MAXCNT) 
		{
			VBAT_Prot.status=2; //欠压状态
			// gSystem_info.Perbat=0;
			// VBAT_Prot.overcount=0;
			// STM_CHG_STOP_OFF;
			// gSystem_info.Error_Category|=1<<3;
		}

	}
	else if((VBAT_Prot.currentValue>=16.5f)&&(VBAT_Prot.currentValue<VBAT_Prot.OverValue)) //充满
	{
        VBAT_Prot.overcount++;
        if(VBAT_Prot.overcount>=TIME_MAXCNT)
        {
            // gSystem_info.Error_Category&=~(1<<2);
            // gSystem_info.Error_Category&=~(1<<3);
            // gSystem_info.Perbat=100;
            VBAT_Prot.status=3;
            VBAT_Prot.overcount=0;
            VBAT_Prot.undercount=0;
        }

	}
	else{
		// gSystem_info.Error_Category&=~(1<<2);
		// gSystem_info.Error_Category&=~(1<<3);
		// gSystem_info.Perbat=(uint8_t)((VBAT_Prot.currentValue-VBAT_Prot.UnderValue)*25);
		VBAT_Prot.status=0;
	//	STM_CHG_STOP_OFF;
		VBAT_Prot.overcount=0;
		VBAT_Prot.undercount=0;
	}	
  	/**************风机电流保护检测**************/
	if(Motorfan.M_Prot.currentValue>=Motorfan.M_Prot.OverValue){
		Motorfan.M_Prot.overcount++;
		if(Motorfan.M_Prot.overcount>=TIME_MAXCNT) Motorfan.M_Prot.status=1; //过流状态
		Motorfan.M_Prot.undercount=0;	
		Motorfan.M_Prot.resumecount=0;
		// gSystem_info.Error_Category|=1<<4;

	}
	else if(Motorfan.M_Prot.currentValue<Motorfan.M_Prot.UnderValue)
	{
		// if(gSystem_info.Motor_Dev.FanMotor.M_Prot->is_moving)
		// {
		// 		Motorfan.M_Prot.undercount++;
		// 		if(Motorfan.M_Prot.undercount>=TIME_MAXCNT) Motorfan.M_Prot.status=2; //开路
		// 		Motorfan.M_Prot.overcount=0;
		// 		Motorfan.M_Prot.resumecount=0;
		// 		// gSystem_info.Error_Category|=1<<5;
		// }
		// else {
		// 		Motorfan.M_Prot.resumecount++;
		// 		if(Motorfan.M_Prot.resumecount>=5*TIME_MAXCNT)
		// 		{
		// 				// gSystem_info.Error_Category&=~(1<<4);
		// 				// gSystem_info.Error_Category&=~(1<<5);
		// 				Motorfan.M_Prot.status=0;
		// 				Motorfan.M_Prot.overcount=0;
		// 				Motorfan.M_Prot.resumecount=0;
		// 				Motorfan.M_Prot.undercount=0;
		// 		}
		// }
	}
	else{
		Motorfan.M_Prot.resumecount++;
		if(Motorfan.M_Prot.resumecount>=5*TIME_MAXCNT)
		{
				// gSystem_info.Error_Category&=~(1<<4);
				// gSystem_info.Error_Category&=~(1<<5);
				Motorfan.M_Prot.status=0;
				Motorfan.M_Prot.overcount=0;
				Motorfan.M_Prot.resumecount=0;
				Motorfan.M_Prot.undercount=0;
		}

	}

}

}
