#include "imu_sensor.h"
#include "peripherals_server/main/device_manager.h"
#include "peripherals_server/base/jkey_common.h"

#define MSG_IMU_TIMING 0x107
std::mutex tex;
std::condition_variable cv;
bool ready=false;

IMU_VALID_DATA  CommParam::ImuData;
IMU_REAL        CommParam::ImuReal;
bool CommParam::ImuRdflag=false;
namespace peripherals{
DP_MSG_HDR(ImuManager,DP_MSG_DEAL,pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[]={
    {"imu_calibrate",   &ImuManager::Imu_Calibrate},
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);
int ImuManager::imu_fd=-1;
int ImuManager::imu_calibrate_flag=0;

float Pi=3.1415926f;
float gs=9.80665f;
static float Gravity2Accel=gs/16384;
static float Degree2Radian=Pi/180/16.4f; //角度转弧度 Pi/180，除以16.4 IMU的分别率
tp prevT;
tp curT;
dd delta;
static uint8_t test_a=0,test_g=0;
float ax_all=0,ay_all=0,az_all=0,gx_all=0,gy_all=0,gz_all=0;
unsigned short time_a=0,time_g=0;
static int angle_val = 0;
static int angle_val_x = 0;
static int angle_val_y = 0;
static float ax_val,ay_val,az_val,gx_val,gy_val,gz_val;
float acc_angle_x = 0;
float acc_angle_y = 0;
float acc_angle_z = 0;

float angleee_x = 0;
float angleee_y = 0;
float angleee = 0;
bool Imu_StartCal;
void posture_cal(double ts_diff);

KFP KFP_ax={0.02,0,0,0,0.001,0.00543};
KFP KFP_ay={0.02,0,0,0,0.001,0.00543};
KFP KFP_az={0.02,0,0,0,0.001,0.00543};
KFP KFP_gx={0.02,0,0,0,0.001,0.00543};
KFP KFP_gy={0.02,0,0,0,0.001,0.00543};
KFP KFP_gz={0.02,0,0,0,0.001,0.00543};

float kalmanFilter(KFP *kfp,float input)
{
    //?��????��???��?????k?��????????????��??? = k-1?��??????????��??? + ???????��??��???
    kfp->Now_P = kfp->LastP + kfp->Q;
    //?��???��????��??????��???��???? = k?��????????????��??? / ?��k?��????????????��??? + ???????��??��?????
    kfp->Kg = kfp->Now_P / (kfp->Now_P + kfp->R);
    //?��??��?????��?????k?��??��???��?????��????? = ��???��??????��???? + ?��???��???? * ?��?????? - ��???��??????��??????
    kfp->out = kfp->out + kfp->Kg * (input -kfp->out);//?��???????????��??????????????????????
    //?��????��???��???: ��???????????��??????? kfp->LastP ????????????��?��???
    kfp->LastP = (1-kfp->Kg) * kfp->Now_P;
    return kfp->out;
}

//构造函数里一般只做初始化变量用
ImuManager::ImuManager(PerServer *per_server_ptr)
    : BaseModule(per_server_ptr),dp_client_(nullptr) {
    Imu_StartCal=false;
    dp_client_ = Perptr_->GetDpClientPtr();
    //从db_manager中获取imu offset
    std::string str_req,str_res;
    Json::Value json_req,json_res;
    json_req[JKEY_TYPE] = "get_imu_offset";
    Json2String(json_req, str_req);
    dp_client_->SendDpMessage("DP_DB_REQUEST", 0, str_req.c_str(), str_req.size(), &str_res);
    //printf("%s\n",str_res.c_str());
    if(String2Json(str_res,json_res)&&(json_res["state"].asInt()==200)) {
        ImuData.gyro_x_offset = json_res["body"]["gyro_x_offset"].asInt();
        ImuData.gyro_y_offset = json_res["body"]["gyro_y_offset"].asInt();
        ImuData.gyro_z_offset = json_res["body"]["gyro_z_offset"].asInt();
        ImuData.accel_x_offset = json_res["body"]["accel_x_offset"].asInt();
        ImuData.accel_y_offset = json_res["body"]["accel_y_offset"].asInt();
        ImuData.accel_z_offset = json_res["body"]["accel_z_offset"].asInt();
        DLOG_INFO(MOD_EB, "gyro_x_offset=%d,gyro_y_offset=%d,gyro_z_offset=%d,\
        accel_x_offset=%d,accel_y_offset=%d,accel_z_offset=%d\n",\
        ImuData.gyro_x_offset,ImuData.gyro_y_offset,ImuData.gyro_z_offset,ImuData.accel_x_offset,ImuData.accel_y_offset,ImuData.accel_z_offset);
    }
}

ImuManager::~ImuManager() {
    
}

int ImuManager::Start() {
   
    DLOG_DEBUG(MOD_EB, "init imu dev\n");
    if(access(DEVIMU_PATH,R_OK)!=0) //检查设备节点是否存在
    {
        DLOG_ERROR(MOD_EB, "imu dev is not exist!\n");
        return -1;
    }
    imu_fd=open(DEVIMU_PATH,O_RDWR);
    if(imu_fd<0)
	{
        close(imu_fd);
        DLOG_ERROR(MOD_EB, "open %s dev error!\n",DEVIMU_PATH);
		return -1;
	} 
    event_service_ = chml::EventService::CreateEventService(NULL, "imu_read");
    ASSERT_RETURN_FAILURE(!event_service_, -1);
    event_service_->PostDelayed(100, this, MSG_IMU_TIMING);   
  //  Imu_Run();
    return 0; 
}

int ImuManager::Stop() {
    DLOG_ERROR(MOD_EB, "close gpioin dev\n");
    close(imu_fd); 
    return 0; 
}

int ImuManager::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
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

int ImuManager::Imu_Calibrate(Json::Value &jreq, Json::Value &jret) {
   imu_calibrate_flag=1;
   return 0;
}

void ImuManager::Imu_Run()
{
    int ret=0;
    int *databuf =  new int[7];
    FD_ZERO(&readfds);
    FD_SET(imu_fd, &readfds);
    std::unique_lock<std::mutex> lock(tex);
    /* 构造超时时间 */
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000; /* 50ms */
    ret = select(imu_fd + 1, &readfds, NULL, NULL, &timeout); 
    switch(ret) {
        case 0: 	/* 超时 */
            /* 用户自定义超时处理 */
            break;
        case -1:	/* 错误 */
            /* 用户自定义错误处理 */
            break;
        default:  /* 可以读取数据 */
            if(FD_ISSET(imu_fd, &readfds))
            {
                curT = sc::now();
                delta=curT-prevT;           
                ret = read(imu_fd, databuf, sizeof(databuf));
                if(ret < 0){
                    /* 读取错误 */
                    DLOG_ERROR(MOD_EB, "read error\r\n");
                }
                else { 			/* 数据读取成功 */
                    if(imu_calibrate_flag!=0) {
                        memset(&ImuData,0,sizeof(ImuData));
                        for(int i=0;i<100;++i)
                        {
                            ImuData.gyro_x_offset += databuf[0];
                            ImuData.gyro_y_offset += databuf[1];
                            ImuData.gyro_z_offset += databuf[2];
                            ImuData.accel_x_offset += databuf[3];
                            ImuData.accel_y_offset += databuf[4];
                            ImuData.accel_z_offset += databuf[5];
                        }
                        ImuData.gyro_x_offset = ImuData.gyro_x_offset/100;
                        ImuData.gyro_y_offset = ImuData.gyro_y_offset/100;
                        ImuData.gyro_z_offset = ImuData.gyro_z_offset/100;
                        ImuData.accel_x_offset = ImuData.accel_x_offset/100;
                        ImuData.accel_y_offset = ImuData.accel_y_offset/100;
                        ImuData.accel_z_offset = ImuData.accel_z_offset/100-16384;
                        imu_calibrate_flag=0;  
                        //设置到db_manager中
                        std::string str_req,str_res;
                        Json::Value json_req;
                        json_req[JKEY_TYPE] = "set_imu_offset";
                        json_req[JKEY_BODY]["gyro_x_offset"]=ImuData.gyro_x_offset; 
                        json_req[JKEY_BODY]["gyro_y_offset"]=ImuData.gyro_y_offset;
                        json_req[JKEY_BODY]["gyro_z_offset"]=ImuData.gyro_z_offset;
                        json_req[JKEY_BODY]["accel_x_offset"]=ImuData.accel_x_offset;
                        json_req[JKEY_BODY]["accel_y_offset"]=ImuData.accel_y_offset;
                        json_req[JKEY_BODY]["accel_z_offset"]=ImuData.accel_z_offset;
                        dp_client_->SendDpMessage("DP_DB_REQUEST", 0, str_req.c_str(), str_req.size(), &str_res);
                    }
                    else {
                        ImuData.gyro_x_adc = databuf[0];
                        ImuData.gyro_y_adc = databuf[1];
                        ImuData.gyro_z_adc = databuf[2];
                        ImuData.accel_x_adc = databuf[3];
                        ImuData.accel_y_adc = databuf[4];
                        ImuData.accel_z_adc = databuf[5];
                        ImuData.temp_adc = databuf[6];
                        
                        /* 计算实际值 */
                        ImuReal.accel_x_act = (float)(ImuData.accel_x_adc-ImuData.accel_x_offset)*Gravity2Accel;
                        ImuReal.accel_y_act = (float)(ImuData.accel_y_adc-ImuData.accel_y_offset)*Gravity2Accel;
                        ImuReal.accel_z_act = (float)(ImuData.accel_z_adc-ImuData.accel_z_offset)*Gravity2Accel;
                        ImuReal.gyro_x_act = (float)(ImuData.gyro_x_adc-ImuData.gyro_x_offset)*Degree2Radian;
                        ImuReal.gyro_y_act = (float)(ImuData.gyro_y_adc-ImuData.gyro_y_offset)*Degree2Radian;
                        ImuReal.gyro_z_act = (float)(ImuData.gyro_z_adc-ImuData.gyro_z_offset)*Degree2Radian; //转换成弧度值

                        ImuReal.temp_act = ((float)(ImuData.temp_adc))/128+ 25;
                        if(!Imu_StartCal) {
                            Imu_StartCal=true;
                        }else {
                            posture_cal(delta.count()/1000);
                            ImuReal.course_angle=(float)angle_val;
                        }
                        ImuRdflag=true;
                        SensorBroadcast();
                        // ready=true;
                        // cv.notify_all();
                    //     DLOG_INFO(MOD_EB,"course_angle = %5.3f\r\n", ImuReal.course_angle);
                    //    DLOG_INFO(MOD_EB,"act gx = %.2frad/S, act gy = %.2frad/S, act gz = %.2frad/S\r\n", ImuReal.gyro_x_act, ImuReal.gyro_y_act, ImuReal.gyro_z_act);
                    //    DLOG_INFO(MOD_EB,"act ax = %.2fm/s2, act ay = %.2fm/s2, act az = %.2fm/s2\r\n", ImuReal.accel_x_act, ImuReal.accel_y_act, ImuReal.accel_z_act);
                    }         
                }
                prevT=curT;
            } 
        }
    delete[] databuf;
}

void ImuManager::OnMessage(chml::Message* msg) {
    if (MSG_IMU_TIMING == msg->message_id) {
    Imu_Run();
    event_service_->Post(this, MSG_IMU_TIMING);
  } 
}

// //积分计算 
// void calculate_imu(uint8_t buf)
// {
//     if(accel_valid){
//         ts_gap = (float)(curr_ts - accel_ts)/1000000.f;
//         ts_gap_prev_a = (float)(accel_ts - accel_prev_ts)/1000000.f;
//         accel_prev_ts = accel_ts;
//         body_lsb[0] = accel[0] * accel_orient[0] + accel[1] * accel_orient[1] + accel[2] * accel_orient[2];
//         body_lsb[1] = accel[0] * accel_orient[3] + accel[1] * accel_orient[4] + accel[2] * accel_orient[5];
//         body_lsb[2] = accel[0] * accel_orient[6] + accel[1] * accel_orient[7] + accel[2] * accel_orient[8];
//         body_conv[0] = (double)body_lsb[0] * accel_scale;
//         body_conv[1] = (double)body_lsb[1] * accel_scale;
//         body_conv[2] = (double)body_lsb[2] * accel_scale;

//         ax= kalmanFilter(&KFP_ax,body_conv[0]);
//         ay= kalmanFilter(&KFP_ay,body_conv[1]);
//         az= kalmanFilter(&KFP_az,body_conv[2]);        
//     }    
// }

void ImuManager::posture_cal(double ts_diff) //ctime--时间间隔
{
    float ax,ay,az,gx,gy,gz;
    ax= kalmanFilter(&KFP_ax,ImuReal.accel_x_act);
    ay= kalmanFilter(&KFP_ay,ImuReal.accel_y_act);
    az= kalmanFilter(&KFP_az,ImuReal.accel_z_act);
    if(test_a == 0)
    {
        ax_all+=ax;
        ay_all+=ay;
        az_all+=az;
        time_a++;
        if(time_a == 100)
        {
            ax_all=ax_all/100;
            ay_all=ay_all/100;
            az_all=az_all/100; 
            test_a = 1;
        }
    }
    else 
    {
        ax=ax-ax_all;
        ay=ay-ay_all;
        az=az-az_all;
    }
    ax_val = ax;
    ay_val = ay;
    az_val = az;
    gx= kalmanFilter(&KFP_gx,ImuReal.gyro_x_act);
    gy= kalmanFilter(&KFP_gy,ImuReal.gyro_y_act);
    gz= kalmanFilter(&KFP_gz,ImuReal.gyro_z_act);
    if(test_g == 0)
    {
        gx_all+=gx;
        gy_all+=gy;
        gz_all+=gz;
        time_g++;
        if(time_g == 100)
        {
            gx_all=gx_all/100;
            gy_all=gy_all/100;
            gz_all=gz_all/100; 
            test_g = 1;
        }
    }
    else
    {       
        gx = gx-gx_all;
        gy = gy-gy_all;
        gz = gz-gz_all;
    }
    if(test_g == 1 && test_a == 1)
    {
        // MahonyAHRSupdateIMU(ax,ay,az,gx,gy,gz,ts_gap_prev_g,ts_gap_prev_a);
        if(gz*gz < 0.00002)
        {


        }
        else
        {
            angleee += gz*ts_diff/1000.f *180.f/Pi;    
            if(angleee > 180.f)
            {
                angleee = angleee -360.f;	
            }
            else if(angleee < -180.f)
            {
                angleee = angleee + 360.f;	
            }      
            angle_val = (int)(angleee*100);
        }
        /**********************添加 加速度计算角度 并融合***************************************/
        acc_angle_x = (atan(ax/sqrt(ay*ay+az*az)))*57.3f*100;  //与x的夹角 --> pitch
        acc_angle_y = (atan(ay/sqrt(ax*ax+az*az)))*57.3f*100;  //与y的夹角 --> yaw
        acc_angle_z = (atan(az/sqrt(ax*ax+ay*ay)))*57.3f*100;  //与z的夹角 --> roll
        /***********************************************************************************/
        if (gx * gx < 0.00002)
        {
        }
        else
        {
            angleee_x += gx * ts_diff / 1000.f * 180.f / Pi;
            if (angleee_x > 180.f)
            {
                angleee_x = angleee_x - 360.f;
            }
            else if (angleee_x < -180.f)
            {
                angleee_x = angleee_x + 360.f;
            }
            angle_val_x = (int)(angleee_x * 100);
        }

        if (gy * gy < 0.00002)
        {
        }
        else
        {
            angleee_y += gy * ts_diff / 1000.f * 180.f / Pi;
            if (angleee_y > 180.f)
            {
                angleee_y = angleee_y - 360.f;
            }
            else if (angleee_y < -180.f)
            {
                angleee_y = angleee_y + 360.f;
            }
            angle_val_y = (int)(angleee_y * 100);
        }
    }
    gx_val  =gx;
    gy_val	=gy;
    gz_val	=gz;
   //printf("ax_val=%f,ay_val=%f,az_val=%f,gx_val=%f,gy_val=%f,gz_val=%f,angle_val=%f\n",ax_val,ay_val,az_val,gx_val,gy_val,gz_val,angle_val);
}

void ImuManager::SensorBroadcast() {
    std::string str_req,str_res;
    Json::Value json_req;
    uint32_t battery;
    /*条件变量等待*/
    // std::unique_lock<std::mutex> lock(tex);
    // cv.wait(lock, []{return ready;});
    // if(ImuRdflag) {

      json_req[JKEY_TYPE] = "sensor_data";
      //vl53l5cx数据
      Json::Value& json_data = json_req[JKEY_BODY]["vl53l5cx"]["data"];
      for(int i=0;i<192;i++) {
        json_data.append(vl53_dis[i]);
      }
      //vl6180数据
      json_req[JKEY_BODY]["vl6180"]["dis"] = vl6180_dis;
      //imu数据
      Json::Value& json_imu = json_req[JKEY_BODY]["imu"];
      json_imu["x_accel"] = ImuReal.accel_x_act;
      json_imu["y_accel"] = ImuReal.accel_y_act;
      json_imu["z_accel"] = ImuReal.accel_z_act;
      json_imu["x_gyro"] = ImuReal.gyro_x_act;
      json_imu["y_gyro"] = ImuReal.gyro_y_act;
      json_imu["z_gyro"] = ImuReal.gyro_z_act;
      json_imu["course_angle"] = ImuReal.course_angle;
      //超声波地毯检测数据
      json_req[JKEY_BODY]["ultra"]["rug"] = rug;
      //电机电流检测数据
      Json::Value& json_curr = json_req[JKEY_BODY]["current"];
      json_curr["wheel_l_curr"] = LeftWheel.M_Prot.currentValue;
      json_curr["wheel_r_curr"] = RightWheel.M_Prot.currentValue;
      json_curr["fan_curr"] = Motorfan.M_Prot.currentValue;
      json_curr["mid_curr"] = Motormid.M_Prot.currentValue;
      json_curr["side_l_curr"] = MotorsideA.M_Prot.currentValue;
      json_curr["side_r_curr"] = MotorsideB.M_Prot.currentValue;
      json_curr["mop_curr"] = MotorMop.M_Prot.currentValue;
      //电量数据
      if(VBAT_Prot.currentValue<12.5f) {
        battery=0;
      }
      else if(VBAT_Prot.currentValue > 16.5f) {
        battery=100;
      }
      else {
        battery=(VBAT_Prot.currentValue-12.5)*100/4;
      }
      json_req[JKEY_BODY]["battery"]["electric_qy"] =battery;
      //地检检测数据
      Json::Value& json_gnddet = json_req[JKEY_BODY]["gnd-det"];
      json_gnddet["gnddet_lf"] = GND_Sensor_LF.readVal;
      json_gnddet["gnddet_lb"] = GND_Sensor_LB.readVal;
      json_gnddet["gnddet_rf"] = GND_Sensor_RF.readVal;
      json_gnddet["gnddet_rb"] = GND_Sensor_RB.readVal; 
      //地检检测数据
      Json::Value& json_odm = json_req[JKEY_BODY]["odm"];
      json_odm["odm_left"] = gODM_CAL.leftv;
      json_odm["odm_right"] = gODM_CAL.rightv;
      json_odm["odm_v"] = gODM_CAL.p_vx;
      json_odm["odm_w"] = gODM_CAL.p_vyaw;
      //报警信号
      json_req[JKEY_BODY]["alarm"]["abnormal"] = abnormal;
      //撞板信号
      Json::Value& json_col = json_req[JKEY_BODY]["collision"];
      json_col["collision_lf"] = (Json::Int)Gpio_PinD.at("COLLISION_DET_LA");
      json_col["collision_lb"] = (Json::Int)Gpio_PinD.at("COLLISION_DET_LB");
      json_col["collision_rf"] = (Json::Int)Gpio_PinD.at("COLLISION_DET_RA");
      json_col["collision_rb"] = (Json::Int)Gpio_PinD.at("COLLISION_DET_RB"); 
      //其他霍尔
      Json::Value& json_hall = json_req[JKEY_BODY]["triggle"];
      json_hall["radar_triggle"]= 0;
      json_hall["visor_det"]= 0;
      json_hall["lidar_pi_key1"]= (Json::Int)Gpio_PinD.at("LIADR_PI_KEYI1");
      json_hall["lidar_pi_key2"]= (Json::Int)Gpio_PinD.at("LIADR_PI_KEYI2");
      //红外数据
      Json::Value& json_ir = json_req[JKEY_BODY]["ir_back"];
      json_ir["left_ir"]=left_irrev;
      json_ir["right_ir"]=right_irrev;
      Json2String(json_req, str_req);
    //  printf("%s\n",str_req.c_str());
      dp_client_->SendDpMessage("DP_PERIPHERALS_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
      //ready=false;
      ImuRdflag=false;    	
}


}
// pthread_mutex_t g_imu_lock;
// int client_imu_send(U8* buf,int len){
// 	pthread_mutex_lock(&g_imu_lock);
// 	int isend =write(socket_imu_id,buf,len);
// 	pthread_mutex_unlock(&g_imu_lock);
// //	LOG_INFO("client_imu_send");
// 	return isend;
// }

// //低位放低字节(和小端模式一致),验证通过
// void serialize_short(U16 data,char* out,int *len){
// 	char *p = out;		
// 	p[0] = data & 0xff; 		//低位放低字节
// 	p[1] = data >> 8 & 0xff;
// 	*len =2;
// }
// //验证通过
// void serialize_imu_data(IMU_VALID_DATA *data,char *out,int *len){
// 	char *p =out;
// 	int skip = sizeof(double);  //8
// 	memcpy(p,&data->vel_angle_x,skip);
// 	p +=skip;
// 	memcpy(p,&data->vel_angle_y,skip);
// 	p +=skip;
// 	memcpy(p,&data->vel_angle_z,skip);	
// 	p +=skip;
// 	memcpy(p,&data->acc_x,skip);
// 	p +=skip;
// 	memcpy(p,&data->acc_y,skip);
// 	p +=skip;
// 	memcpy(p,&data->acc_z,skip);
// 	*len = 6*skip;	
// 	for(int i =0;i<*len;i++){
// //		LOG_INFO("serialize_imu_data:%x",out[i]);
// 	}
// }
// U16 crc16check(U8 *data, U32 len)
// {
//     U16 crc_gen = 0xa001;
//     U16 crc;
//     U32 i, j;
//     /*init value of crc*/
//     crc = 0xffff;
//     if (len != 0)
//     {
//         for (i = 0; i < len; i++)
//         {
//             crc ^= (U16)(data[i]);
//             for (j = 0; j < 8; j++)
//             {
//                 if ((crc & 0x01) == 0x01)
//                 {
//                     crc >>= 1;
//                     crc ^= crc_gen;
//                 }
//                 else
//                 {
//                     crc >>= 1;
//                 }
//             }
//         }
//     } 
//     return crc;
// }

// //陀螺仪数据回调,对象字节化后放入c_write_queue
// void imu_callback(double angleX,double angleY,double angleZ,
// 	double accX,double accY,double accZ){	
// 	char buffer[200]={0};
// 	int alen=0,blen=0,clen=8,dlen=0;
// 	U16 head = u8_to_u16(IMU_DATA_HEAD_CHAR2,IMU_DATA_HEAD_CHAR1);
// 	serialize_short(head,buffer,&alen);		//头部序列化,目的是生成校验码
// 	IMU_VALID_DATA imu={angleX,angleY,angleZ,accX,accY,accZ};
// 	serialize_imu_data(&imu,&buffer[alen],&blen);//IMU序列化,目的是生成校验码
// 	long timestamp = getTimeStamp();
// 	memcpy(&buffer[alen + blen],&timestamp,clen);//时间戳	
// 	U16 checkcode= crc16check(buffer,alen + blen+ clen);		
// 	serialize_short(checkcode,&buffer[alen + blen+ clen],&dlen);//校验码序列化
// //	LOG_INFO("imu_callback:[%lf,%lf,%lf],[%lf,%lf,%lf],%ld",
// //			angleX,angleY,angleZ,accX,accY,accZ,timestamp);
// 	client_imu_send(buffer,alen+blen+clen+dlen);
// }



