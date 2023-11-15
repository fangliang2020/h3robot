#include "wheel_manager.h"
#include "peripherals_server/main/device_manager.h"
#include "peripherals_server/base/jkey_common.h"

#define LEFT_WHEEL_MAGIC 				'P'
#define LEFT_WHEEL_SET_PERIOD			_IOW(LEFT_WHEEL_MAGIC, 0, int)
#define LEFT_WHEEL_GET_PERIOD			_IOR(LEFT_WHEEL_MAGIC, 1, int)
#define LEFT_WHEEL_SET_A_DUTY			_IOW(LEFT_WHEEL_MAGIC, 2, int)
#define LEFT_WHEEL_SET_B_DUTY			_IOW(LEFT_WHEEL_MAGIC, 3, int)
#define LEFT_WHEEL_GET_DUTY				_IOR(LEFT_WHEEL_MAGIC, 4, int)
#define LEFT_WHEEL_SET_POLARITY			_IOW(LEFT_WHEEL_MAGIC, 5, int)
#define LEFT_WHEEL_GET_POLARITY			_IOR(LEFT_WHEEL_MAGIC, 6, int)
#define LEFT_WHEEL_ENABLE				_IOW(LEFT_WHEEL_MAGIC, 7, int)
#define LEFT_WHEEL_DISABLE				_IOR(LEFT_WHEEL_MAGIC, 8, int)
#define LEFT_WHEEL_GET_SPEED			_IOR(LEFT_WHEEL_MAGIC, 9, int)
#define LEFT_WHEEL_BRAKE				_IOR(LEFT_WHEEL_MAGIC, 10, int)
#define LEFT_WHEEL_MAXNR				10
#define RIGHT_WHEEL_MAGIC 				'P'
#define RIGHT_WHEEL_SET_PERIOD			_IOW(RIGHT_WHEEL_MAGIC, 0, int)
#define RIGHT_WHEEL_GET_PERIOD			_IOR(RIGHT_WHEEL_MAGIC, 1, int)
#define RIGHT_WHEEL_SET_A_DUTY			_IOW(RIGHT_WHEEL_MAGIC, 2, int)
#define RIGHT_WHEEL_SET_B_DUTY			_IOW(RIGHT_WHEEL_MAGIC, 3, int)
#define RIGHT_WHEEL_GET_DUTY			_IOR(RIGHT_WHEEL_MAGIC, 4, int)
#define RIGHT_WHEEL_SET_POLARITY		_IOW(RIGHT_WHEEL_MAGIC, 5, int)
#define RIGHT_WHEEL_GET_POLARITY		_IOR(RIGHT_WHEEL_MAGIC, 6, int)
#define RIGHT_WHEEL_ENABLE				_IOW(RIGHT_WHEEL_MAGIC, 7, int)
#define RIGHT_WHEEL_DISABLE				_IOR(RIGHT_WHEEL_MAGIC, 8, int)
#define RIGHT_WHEEL_GET_SPEED			_IOR(RIGHT_WHEEL_MAGIC, 9, int)
#define RIGHT_WHEEL_BRAKE				_IOR(RIGHT_WHEEL_MAGIC, 10, int)
#define RIGHT_WHEEL_MAXNR				10

#define Pi 3.14159f
#define ke 0.1f*Pi/180.0f
#define W_perimeter 0.46814f //履带一圈的周长 单位m --还需要实际标定

static uint32_t LENGTH2ENCODE_L=4200; 
static uint32_t LENGTH2ENCODE_R=4183;
static float WHEEL_DIAMETER=0.0795f; // 轮子半径
static float WHEEL_DISTANCE=0.271f; // 轮子间距
static float REDUCTION_RATIO=65.6f; // 减速比
static float ENCODE_NUMS=30.0f; // 电机编码器线数
static int16_t last_left_encoder = 0;
static int16_t last_right_encoder = 0;
static float coeff_L = 0.f;
static float coeff_R = 0.f;
static float coeff1 = 0.f;
static float half_wheel_distance = 0.f;
static float wheel_radius_inv_L = 0.f;
static float wheel_radius_inv_R = 0.f;
static float coeff2 =0.f;
static float ENCODE_RATIO_L= 0.f; //左轮一圈对应的码盘数
static float ENCODE_RATIO_R= 0.f; //右轮一圈对应的码盘数 --还需要实际标定


ODM_Cal CommParam::gODM_CAL;

namespace peripherals{

#define MSG_WHEEL_TIMING 0x102

DP_MSG_HDR(WheelManager,DP_MSG_DEAL,pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[]={
    {"set_v_w",   &WheelManager::SetMoveCtrl},
		{"odm_calibrate", &WheelManager::odm_calibrate},
};

static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);


WheelManager::WheelManager(PerServer *per_server_ptr) 
	: BaseModule(per_server_ptr),dp_client_(nullptr) {
  odometry_init(0,0);
	memset(&gODM_CAL,0,sizeof(gODM_CAL));
	Event_Service_=Perptr_->GetEventServicePtr();
	dp_client_ = Perptr_->GetDpClientPtr();
	//从db_manager中获取imu offset
	std::string str_req,str_res;
	Json::Value json_req,json_res;
	json_req[JKEY_TYPE] = "get_wheel_pid";
	Json2String(json_req, str_req);
	dp_client_->SendDpMessage("DP_DB_REQUEST", 0, str_req.c_str(), str_req.size(), &str_res);
	DLOG_INFO(MOD_EB,"%s\n",str_res.c_str());
	if(String2Json(str_res,json_res)&&(json_res["state"].asInt()==200)) {
		Json::Value json_pid = json_res["body"];
		pidValue[0].speed=json_pid["pid"][0]["DIF"].asInt();
		pidValue[0].kp=json_pid["pid"][0]["KP"].asFloat();
		pidValue[0].ki=json_pid["pid"][0]["KI"].asFloat();
		pidValue[0].kd=json_pid["pid"][0]["KD"].asFloat();
		pidValue[0].iLimit=200;
		pidValue[1].speed=json_pid["pid"][1]["DIF"].asInt();
		pidValue[1].kp=json_pid["pid"][1]["KP"].asFloat();
		pidValue[1].ki=json_pid["pid"][1]["KI"].asFloat();
		pidValue[1].kd=json_pid["pid"][1]["KD"].asFloat();
		pidValue[1].iLimit=500;
		pidValue[2].speed=json_pid["pid"][2]["DIF"].asInt();
		pidValue[2].kp=json_pid["pid"][2]["KP"].asFloat();
		pidValue[2].ki=json_pid["pid"][2]["KI"].asFloat();
		pidValue[2].kd=json_pid["pid"][2]["KD"].asFloat();
		pidValue[2].iLimit=1500;
		pidValue[3].speed=json_pid["pid"][3]["DIF"].asInt();
		pidValue[3].kp=json_pid["pid"][3]["KP"].asFloat();
		pidValue[3].ki=json_pid["pid"][3]["KI"].asFloat();
		pidValue[3].kd=json_pid["pid"][3]["KD"].asFloat();
		pidValue[3].iLimit=2000;
		DLOG_INFO(MOD_EB, "pidValue[0].speed=%d,pidValue[0].kp=%5.3f,pidValue[0].ki=%5.3f,pidValue[0].kd=%5.3f,\n\
		pidValue[1].speed=%d,pidValue[1].kp=%5.3f,pidValue[1].ki=%5.3f,pidValue[1].kd=%5.3f,\n\
		pidValue[2].speed=%d,pidValue[2].kp=%5.3f,pidValue[2].ki=%5.3f,pidValue[2].kd=%5.3f,\n\
		pidValue[3].speed=%d,pidValue[3].kp=%5.3f,pidValue[3].ki=%5.3f,pidValue[3].kd=%5.3f\n",\
		pidValue[0].speed,pidValue[0].kp,pidValue[0].ki,pidValue[0].kd,\
		pidValue[1].speed,pidValue[1].kp,pidValue[1].ki,pidValue[1].kd,\
		pidValue[2].speed,pidValue[2].kp,pidValue[2].ki,pidValue[2].kd,\
		pidValue[3].speed,pidValue[3].kp,pidValue[3].ki,pidValue[3].kd);
	}	
	else {
		memcpy(&pidValue,&pidValue_last,sizeof(pidValue));
		DLOG_INFO(MOD_EB, " copy pidvalue_last to pidvalue,\n\
		pidValue[0].speed=%d,pidValue[0].kp=%5.3f,pidValue[0].ki=%5.3f,pidValue[0].kd=%5.3f,\n\
		pidValue[1].speed=%d,pidValue[1].kp=%5.3f,pidValue[1].ki=%5.3f,pidValue[1].kd=%5.3f,\n\
		pidValue[2].speed=%d,pidValue[2].kp=%5.3f,pidValue[2].ki=%5.3f,pidValue[2].kd=%5.3f,\n\
		pidValue[3].speed=%d,pidValue[3].kp=%5.3f,pidValue[3].ki=%5.3f,pidValue[3].kd=%5.3f\n",\
		pidValue[0].speed,pidValue[0].kp,pidValue[0].ki,pidValue[0].kd,\
		pidValue[1].speed,pidValue[1].kp,pidValue[1].ki,pidValue[1].kd,\
		pidValue[2].speed,pidValue[2].kp,pidValue[2].ki,pidValue[2].kd,\
		pidValue[3].speed,pidValue[3].kp,pidValue[3].ki,pidValue[3].kd);
	}
	json_req[JKEY_TYPE] = "get_wheel_param";
	Json2String(json_req, str_req);
	dp_client_->SendDpMessage("DP_DB_REQUEST", 0, str_req.c_str(), str_req.size(), &str_res);
	DLOG_INFO(MOD_EB,"%s\n",str_res.c_str());
	if(String2Json(str_res,json_res)&&(json_res["state"].asInt()==200)) {
		Json::Value json_param = json_res["body"];
		ENCODE_NUMS=	json_param["wheel_code_disc"].asFloat();	
		WHEEL_DISTANCE= json_param["wheel_distance"].asFloat();
		WHEEL_DIAMETER= json_param["wheel_diameter"].asFloat(); 
		LENGTH2ENCODE_L= json_param["length2encode_l"].asUInt(); 
		LENGTH2ENCODE_R= json_param["length2encode_r"].asUInt();
		DLOG_INFO(MOD_EB, "wheel_code_disc=%3.5f,wheel_distance=%3.5f,wheel_diameter=%3.5f,\
        length2encode_l=%d,length2encode_r=%d\n",ENCODE_NUMS,WHEEL_DISTANCE,WHEEL_DIAMETER,LENGTH2ENCODE_L,LENGTH2ENCODE_R);
	}
	coeff_L = WHEEL_DIAMETER * M_PI_ / (float)(ENCODE_NUMS * REDUCTION_RATIO);
	coeff_R = WHEEL_DIAMETER * M_PI_ / (float)(ENCODE_NUMS * REDUCTION_RATIO);
	coeff1 = 1.0f / WHEEL_DISTANCE;
	half_wheel_distance = WHEEL_DISTANCE * 0.5f;
	wheel_radius_inv_L = 1.0f / (WHEEL_DIAMETER * 0.5f);
	wheel_radius_inv_R = 1.0f / (WHEEL_DIAMETER * 0.5f);
	coeff2 = ENCODE_NUMS*REDUCTION_RATIO/ (2.0f * M_PI_);
	ENCODE_RATIO_L=W_perimeter/LENGTH2ENCODE_L; //左轮一圈对应的码盘数
	ENCODE_RATIO_R=W_perimeter/LENGTH2ENCODE_R; //右轮一圈对应的码盘数 --还需要实际标定
}
WheelManager::~WheelManager(){

}

int WheelManager::Start() {
	lwheel_fd=-1;
	rwheel_fd=-1;
	DLOG_DEBUG(MOD_EB, "init lwheel motor\n");
	if(access(DEVLWHEEL_PATH,F_OK)!=0)
	{
		DLOG_ERROR(MOD_EB, "lwheel motor is not exist!\n");
    return -1;
	}
	lwheel_fd=open(DEVLWHEEL_PATH,O_RDWR);
	if(lwheel_fd<0)
	{
		close(lwheel_fd);
		DLOG_ERROR(MOD_EB, "open %s dev error!\n",DEVLWHEEL_PATH);
		return -1;
	}
	DLOG_DEBUG(MOD_EB, "init rwheel motor\n");
	if(access(DEVRWHEEL_PATH,F_OK)!=0)
	{
		DLOG_ERROR(MOD_EB, "rwheel motor is not exist!\n");
    return -1;
	}
	rwheel_fd=open(DEVRWHEEL_PATH,O_RDWR);
	if(rwheel_fd<0)
	{
		close(rwheel_fd);
		DLOG_ERROR(MOD_EB, "open %s dev error!\n",DEVRWHEEL_PATH);
		return -1;
	}
	ioctl(lwheel_fd,LEFT_WHEEL_ENABLE,1);
	ioctl(rwheel_fd,RIGHT_WHEEL_ENABLE,1);
	Event_Service_->PostDelayed(50, this, MSG_WHEEL_TIMING);
	return 0;
}

int WheelManager::Stop()
{
	DLOG_DEBUG(MOD_EB, "close lwheel motor\n");
	ioctl(lwheel_fd,LEFT_WHEEL_BRAKE,1);
	ioctl(lwheel_fd,LEFT_WHEEL_DISABLE,1);
	close(lwheel_fd);
	DLOG_DEBUG(MOD_EB, "close rwheel motor\n");
	ioctl(rwheel_fd,RIGHT_WHEEL_BRAKE,1);
	ioctl(rwheel_fd,RIGHT_WHEEL_DISABLE,1);
	close(rwheel_fd);
	return 0;
}

int WheelManager::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
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

void WheelManager::OnMessage(chml::Message* msg) {
	if(MSG_WHEEL_TIMING == msg->message_id) {
		Timer20ms();
		Event_Service_->PostDelayed(20,this,MSG_WHEEL_TIMING);
	}
}

void WheelManager::Timer20ms() {
	CodeWheel_Count();
  wheel_tick_ctrl();
}

int WheelManager::SetMoveCtrl(Json::Value &jreq, Json::Value &jret) //设置V和W。
{
	int JV_TO_INT(jreq,"brake",brake,0); 
	int JV_TO_INT(jreq,"v",v,0);
	int JV_TO_INT(jreq,"w",w,0);
  if((fabs(v)<1e-5)&&(fabs(w)<1e-5)) {
		LeftWheel.workstate=0;
		LeftWheel.setspeed=0;
		LeftWheel.readspeed=0;
		Wheel_Ctrl_Left(0,0,0);
		MotorPidclear(&lMotorPid);
		RightWheel.workstate=0;
		RightWheel.setspeed=0;
		RightWheel.readspeed=0;
		Wheel_Ctrl_Right(0,0,0);
		MotorPidclear(&rMotorPid);
		return 0;					
	}
	gODM_CAL.vx=(float)v*0.001f;
	gODM_CAL.vyaw=(float)w*0.01f;
	gODM_CAL.vy=0.f;
	DLOG_INFO(MOD_EB, "v is %2.5f,w is %2.5f\n",gODM_CAL.vx,gODM_CAL.vyaw);
	gODM_CAL.runTime=0.02f;
	wheel_spd_calc(&gODM_CAL);//计算出setspeed
  return 0;
}

int WheelManager::odm_calibrate(Json::Value &jreq, Json::Value &jret) {
	return 0;
}

void WheelManager::odometry_init(int left_encoder, int right_encoder)
{
	
    coeff_L = gODM_SAVE.Wheel_L_offset * WHEEL_DIAMETER * M_PI_ / (float)(ENCODE_NUMS * REDUCTION_RATIO);
    coeff_R = gODM_SAVE.Wheel_R_offset * WHEEL_DIAMETER * M_PI_ / (float)(ENCODE_NUMS * REDUCTION_RATIO);
    coeff1 = 1.0f / (WHEEL_DISTANCE*gODM_SAVE.Wheel_DIS_offset);
    //half_wheel_distance = WHEEL_DISTANCE*gSysSavePram.Wheel_DIS_offset * 0.5f;
    //wheel_radius_inv_L = 1.0f / (gSysSavePram.Wheel_L_offset * WHEEL_DIAMETER * 0.5f);
    //wheel_radius_inv_R = 1.0f / (gSysSavePram.Wheel_R_offset * WHEEL_DIAMETER * 0.5f);	
    coeff2 = ENCODE_NUMS*REDUCTION_RATIO/ (2.0f * M_PI_);
    last_left_encoder = left_encoder;
    last_right_encoder = right_encoder;
}

void WheelManager::odometry_calc(ODM_Cal* Calodmdata)
{
   int16_t  left_encode_increment;
	 int16_t  right_encode_increment;

	 if((Calodmdata->dcL-last_left_encoder)<-32767) //正走溢出跳变
	 {
		 left_encode_increment=(Calodmdata->dcL+32768)+(32767-last_left_encoder);
	 }
	 else if((Calodmdata->dcL-last_left_encoder)>32767)  //反走跳变
	 {
		 left_encode_increment=(-32768-last_left_encoder)+(Calodmdata->dcL-32767);
	 }
	 else 
	 {
		 left_encode_increment = (Calodmdata->dcL - last_left_encoder);
	 }
	 
	 if((Calodmdata->dcR-last_right_encoder)<-32767) //正走溢出跳变
	 {
		 right_encode_increment=(Calodmdata->dcR+32768)+(32767-last_right_encoder);
	 }
	 else if((Calodmdata->dcR-last_right_encoder)>32767)  //反走跳变
	 {
		 right_encode_increment=(-32768-last_right_encoder)+(Calodmdata->dcR-32767);
	 }
	 else 
	 {
		 right_encode_increment = (Calodmdata->dcR - last_right_encoder);
	 }	 
    float sl = left_encode_increment * coeff_L;      // 左轮行走弧长
    float sr = right_encode_increment * coeff_R;     // 右轮行走弧长
	
	last_left_encoder = Calodmdata->dcL;
	last_right_encoder = Calodmdata->dcR;

    // 直线运动模式
    if (fabs(sr - sl) < 1e-5)
    {
      // result velocity
      Calodmdata->cal_vx = sr / Calodmdata->runTime;
      Calodmdata->cal_vy = 0.f;
      Calodmdata->cal_vw = 0.f;
    }
    else // 圆弧运动模式
    {
      float d_seta = (sr - sl) * coeff1;
      // result velocity
      Calodmdata->cal_vx = (sr + sl) / (2.0f * Calodmdata->runTime);
      Calodmdata->cal_vy = 0.f;
      Calodmdata->cal_vw = d_seta / Calodmdata->runTime;
    }
    // normalize
}

void WheelManager::wheel_spd_calc(ODM_Cal* MoveVMdata)
{
	static uint8_t spdzero_cnt;
	float vl = 0.0f;
	float vr = 0.0f;
	//half_wheel_distance = WHEEL_DISTANCE * 0.5f;
	/* (Vr-Vl)/L=W; (Vr+Vl)/2=V; 
		-> Vr-Vl=W*L;Vr+Vl=2*V;
		-> Vr=(WL+2*V)/2, Vl=(2V-WL)/2;
	*/
	vl=(2*MoveVMdata->vx - MoveVMdata->vyaw*WHEEL_DISTANCE)*0.5f;
	vr=(2*MoveVMdata->vx + MoveVMdata->vyaw*WHEEL_DISTANCE)*0.5f;
    // 直线运动模式
    // if (fabs(MoveVMdata->vyaw) < 1e-5)
    // {
    //     vl = vr = MoveVMdata->vx;
    // }
    // else // 圆弧运动模式
    // {
    //     float R = MoveVMdata->vx / MoveVMdata->vyaw;
    //     vl = (R + half_wheel_distance) * MoveVMdata->vyaw;
    //     vr = (R - half_wheel_distance) * MoveVMdata->vyaw;
    // }
	
		//倒推 V W 关系时使用{
		//     （0.001*V ± (0.00174*W *half_wheel_distance)）*wheel_radius_inv*coeff2*dtime
		//float wd=half_wheel_distance;//0.135
    //float  e =wheel_radius_inv_L*coeff2*dtime; //236.39
		//D10倒推结果 14<=abs(V*0.23639 ± W *0.05569 )<=60

    // n_left / dt = ENCODE_NUMS * w / (2pi)
    LeftWheel.setspeed =  (int)(vl*MoveVMdata->runTime/ENCODE_RATIO_L); //20ms行驶的长度对应的码盘数
    RightWheel.setspeed=  (int)(vr*MoveVMdata->runTime/ENCODE_RATIO_R);
    if((LeftWheel.setspeed==0)&&(RightWheel.setspeed==0))	
	{
		if(spdzero_cnt>=100)
		{
			LeftWheel.workstate=0;
			RightWheel.workstate=0;
		}
		else 
		{
			spdzero_cnt++;
		}
	}
	else 
	{
		spdzero_cnt=0;
		LeftWheel.workstate=1;
		RightWheel.workstate=1;
	}
	DLOG_INFO(MOD_EB, "v= %d,w= %d,LeftWheel.setspeed=%d,RightWheel.setspeed=%d.\n",
	MoveVMdata->vx,MoveVMdata->vyaw,LeftWheel.setspeed,RightWheel.setspeed);

}
/* pulse:占空比
   dir:方向 0-正转 1-反转
   state: 0-停止，1-运行，3-刹车
*/
void WheelManager::Wheel_Ctrl_Left(int pulse,uint8_t dir,uint8_t state)
{
	if(state==0){ //停止和刹车暂定同一种处理
		ioctl(lwheel_fd,LEFT_WHEEL_BRAKE,1);
	}else if(state==3){
		ioctl(lwheel_fd,LEFT_WHEEL_BRAKE,1);
	}else if(state==1){
		if(dir==0){
			ioctl(lwheel_fd,LEFT_WHEEL_SET_A_DUTY,&pulse);
			//printf("LEFT_WHEEL_SET_A_DUTY %d\n",pulse);
		}else if(dir==1){		
			ioctl(lwheel_fd,LEFT_WHEEL_SET_B_DUTY,&pulse);
			//printf("LEFT_WHEEL_SET_B_DUTY %d\n",pulse);
		}
	}
}
void WheelManager::Wheel_Ctrl_Right(int pulse,uint8_t dir,uint8_t state)
{
	// int  duty;
	if(state==0){ //停止和刹车暂定同一种处理
		ioctl(rwheel_fd,RIGHT_WHEEL_BRAKE,1);
	}else if(state==3){
		ioctl(rwheel_fd,RIGHT_WHEEL_BRAKE,1);
	}else if(state==1){
		if(dir==0){
			ioctl(rwheel_fd,RIGHT_WHEEL_SET_B_DUTY,&pulse);			
		}else if(dir==1){
			ioctl(rwheel_fd,RIGHT_WHEEL_SET_A_DUTY,&pulse);
		}
	}
}
void WheelManager::wheel_tick_ctrl()
{
	int32_t Left_OutPwm=0;
	int32_t Right_OutPwm=0;

	if(LeftWheel.workstate==3) //刹车
	{		
		Wheel_Ctrl_Left(0,0,3);
		MotorPidclear(&lMotorPid);
		LeftWheel.readspeed=0;			
	}
	else if(LeftWheel.workstate==1) 
	{		
		if(LeftWheel.M_Prot.status==1)//开路，过流保护
		{
			Wheel_Ctrl_Left(0,0,0);
			MotorPidclear(&lMotorPid);
			LeftWheel.readspeed=0;				
		}
		else
		{
			if(LeftWheel.setspeed==0) {
				Wheel_Ctrl_Left(0,0,0);
				MotorPidclear(&lMotorPid);						
			}	
			else {
				if(LeftWheel.setspeed>=0) {
					if(L_SPD.DIR>0) MotorPidclear(&lMotorPid); //换向时清空pid参数
					L_SPD.DIR=0;
			  }
				else if(LeftWheel.setspeed<0) {
					if(L_SPD.DIR==0) MotorPidclear(&lMotorPid);
					L_SPD.DIR=1;
				}  			
				Left_OutPwm=PidFun_left(&LeftWheel);
				DLOG_INFO(MOD_EB, "LeftWheel.setspeed is %d,Right_OutPwm is %d.\n",
						LeftWheel.setspeed,Left_OutPwm);
				/*****更新PWM的脉宽********/
				Wheel_Ctrl_Left(abs(Left_OutPwm),L_SPD.DIR,1);
			}				
		}			
	}

	if(RightWheel.workstate==3) //刹车
	{	
		Wheel_Ctrl_Right(0,0,3);	
		MotorPidclear(&rMotorPid);
		RightWheel.readspeed=0;				
	}
	else if(RightWheel.workstate==1) 
	{
		if(RightWheel.M_Prot.status==1)//开路，过流保护
		{
			Wheel_Ctrl_Right(0,0,0);
			MotorPidclear(&rMotorPid);			
		}else{
		
			if(RightWheel.setspeed==0) {
				Wheel_Ctrl_Right(0,0,0);
				MotorPidclear(&rMotorPid);
			}
			else {	
				if(RightWheel.setspeed>=0)
				{
					if(R_SPD.DIR>0) MotorPidclear(&rMotorPid); //换向时清空pid参数
					R_SPD.DIR=0;
				}
				else if(RightWheel.setspeed<0)
				{
					if(R_SPD.DIR==0) MotorPidclear(&rMotorPid);
					R_SPD.DIR=1;
				} 				
				/***pid计算得出的数值********/
		   	Right_OutPwm=PidFun_right(&RightWheel);
		  	DLOG_INFO(MOD_EB, "RightWheel.setspeed is %d,Right_OutPwm is %d.\n",
						RightWheel.setspeed,Right_OutPwm);
				/*****更新PWM的脉宽********/
				Wheel_Ctrl_Right(abs(Right_OutPwm),R_SPD.DIR,1);
			}
		}		
	}
}
//0-正向，1-反向
void WheelManager::CodeWheel_Count()
{
	ioctl(lwheel_fd,LEFT_WHEEL_GET_SPEED,&L_SPD.cnt);
	ioctl(rwheel_fd,RIGHT_WHEEL_GET_SPEED,&R_SPD.cnt);
	if(L_SPD.DIR==0){
		LeftWheel.readspeed=L_SPD.cnt;
		//gODM_CAL.dcL=gODM_CAL.dcL+L_SPD.cnt; //码盘累加
	}
	else if(L_SPD.DIR){
		LeftWheel.readspeed=-L_SPD.cnt;
		// gODM_CAL.dcL+=gODM_CAL.dcL-L_SPD.cnt; //码盘累加
	}
	if(R_SPD.DIR==0){
		RightWheel.readspeed=R_SPD.cnt;
		//gODM_CAL.dcR=gODM_CAL.dcR+R_SPD.cnt; //码盘累加
	}
	else if(R_SPD.DIR){
		RightWheel.readspeed=-R_SPD.cnt;
		// gODM_CAL.dcR+=gODM_CAL.dcR-R_SPD.cnt; //码盘累加
	}
	gODM_CAL.leftv=(int)((float)LeftWheel.readspeed*ENCODE_RATIO_L*50000);   //乘以1000换算成mm，除以0.02计算成速度
	gODM_CAL.rightv=(int)((float)RightWheel.readspeed*ENCODE_RATIO_R*50000);


	// DLOG_INFO(MOD_EB, "leftspeed is %d,rightspeed is %d,leftv=%d,rightv=%d.\n",
	// 					LeftWheel.readspeed,RightWheel.readspeed,gODM_CAL.leftv,gODM_CAL.rightv);
	L_SPD.cnt=0;//瞬时速度，每次读取完都要清零
	R_SPD.cnt=0;
	if(fabs(gODM_CAL.leftv-gODM_CAL.rightv)< 1e-5) {
		gODM_CAL.p_vx=gODM_CAL.leftv;
		gODM_CAL.p_vyaw=gODM_CAL.rightv;
	}
	else {
		gODM_CAL.p_vx =  (gODM_CAL.rightv+gODM_CAL.leftv)/2.0f*coeff1;
		gODM_CAL.p_vyaw= (gODM_CAL.rightv-gODM_CAL.leftv)*coeff1;
	}
}
	
}