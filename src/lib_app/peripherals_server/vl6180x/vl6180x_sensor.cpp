#include "vl6180x_sensor.h"
#include <cstdio>
#include <cstring>
#include <cerrno>
#include "peripherals_server/main/device_manager.h"
#include "net_server/base/jkey_common.h"
#define MSG_SAMPLING_TIMING 0x103
static struct input_event inputevent;
uint8_t CommParam::vl6180_dis=255;

#define REG_IDENTIFICATION_MODEL_ID 0x00
#define REG_SYSTEM_FRESH_OUT_OF_RESET 0x0016
#define REG_RESULT_RANGE_STATUS 0x004d
#define REG_RESULT_INTERRUPT_STATUS_GPIO 0x004f
#define REG_SYSTEM_INTERRUPT_CLEAR 0x0015
#define REG_SYSRANGE_START 0x0018
#define REG_RESULT_RANGE_VAL 0x0062

namespace peripherals {

DP_MSG_HDR(Vl6180xManager,DP_MSG_DEAL,pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[]={
    {"board_calibrate",   &Vl6180xManager::Vl6180_Xtakcalib},  //tof校准，需要通过反光片
    {"board_demarcate",   &Vl6180xManager::Vl6180_Offcalib},  //tof标定，需要放置障碍物
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

Vl6180xManager::Vl6180xManager(PerServer *per_server_ptr) 
    : BaseModule(per_server_ptr) {
    VlWk_State=Vl6180WorkState::VL_WORK;
    SamState=WorkMode::SINGLE_DATA;
    event_service_=nullptr;
    dp_client_=Perptr_->GetDpClientPtr();
}
Vl6180xManager::~Vl6180xManager() {
    #if USE_I2C2_PORT
    if (connected) close(i2c_fd);
    #endif
    close(vl6180_fd);
}

int Vl6180xManager::Start() {
    OpenVl6180Normal();
    InitVl6180Dev();
    event_service_=chml::EventService::CreateEventService(NULL,"vl6180_sample");
    ASSERT_RETURN_FAILURE(!event_service_, -1);
    event_service_->PostDelayed(100, this, MSG_SAMPLING_TIMING);
    return 0;
}

int Vl6180xManager::Stop() {
    DeInitVl6180Dev();
    return 0;
}

int Vl6180xManager::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
    DLOG_DEBUG(MOD_EB, "OnDpMessage:%s.", stype.c_str());
    
    int nret = -1;
    for (uint32 idx = 0; idx < g_dpmsg_table_size; ++idx) {
        if (stype == g_dpmsg_table[idx].type) {
        event_service_->Clear(this,MSG_SAMPLING_TIMING);
        DeInitVl6180Dev();
        OpenVl6180Normal();
        InitVl6180Dev();
        nret = (this->*g_dpmsg_table[idx].handler) (jreq["body"], jret);
        if(nret==0) //返回正确
        {
            DLOG_INFO(MOD_EB, "*******************reset vl6180****************\n");
            VlWk_State=VL_HWRESET;
            event_service_->PostDelayed(100, this, MSG_SAMPLING_TIMING);
        }
        else {
            DLOG_INFO(MOD_EB, "&&&&&&&&&&&&&&&&&error reset vl6180&&&&&&&&&&&&&\n");
            DeInitVl6180Dev();
        }
        break;
        }
    }
    
    return nret;   
}

void Vl6180xManager::OnMessage(chml::Message* msg) {
  if (MSG_SAMPLING_TIMING == msg->message_id) {
    Vl6180_Run();
    event_service_->PostDelayed(50, this, MSG_SAMPLING_TIMING);
  }
}
#if USE_I2C2_PORT
int Vl6180xManager::OpenVl6180Normal()
{
    
   std::string filename = "/dev/i2c-2";
  uint8_t devid;
  i2c_fd = open(filename.c_str(), O_RDWR);
  connected = true;
  if (i2c_fd < 0) {
    connected = false;
    std::cout << "VL6180X: Failed to open I2C device at '" << filename << "'\n\t" << strerror(errno) << '\n';
    return 0;
  }
  else {
    std::cout << "VL6180X: Successfully opened I2C device at '" << filename << "'\n";
  }
  
  if (ioctl(i2c_fd, I2C_SLAVE, 0x29) < 0) {
    connected = false;
    std::cout << "VL6180X: Failed to acquire I2C bus address and/or talk to slave.\n\t" << strerror(errno) << '\n';
    return 0;
  }
  
  // Check model id.
    devid=read_register(REG_IDENTIFICATION_MODEL_ID);
  if ( devid!= 0xB4) {
    connected = false;
    std::cout << "VL6180X: Invalid I2C Model ID\n";
    return 0;
  }
  printf("devid is %02x\n",devid);
  // Read SYSTEM__FRESH_OUT_OF_RESET register.
  if (!(read_register(REG_SYSTEM_FRESH_OUT_OF_RESET) & 0x01)) {
      // Ideally we'd reset the device by applying logic '0' to GPIO0, but we can't without that wired up.
      //理想情况下，我们会通过将逻辑“0”应用于GPIO0来重置设备，但如果没有连接，我们就无法重置。
    std::cout << "VL6180X: Not fresh out of reset\n";
    
  //  return;
  }
  
  write_register(REG_SYSTEM_FRESH_OUT_OF_RESET, 0x00);
  
  // Apply the tuning settings.
  write_register(0x0207, 0x01);
  write_register(0x0208, 0x01);
  write_register(0x0096, 0x00);
  write_register(0x0097, 0xfd);
  write_register(0x00e3, 0x00);
  write_register(0x00e4, 0x04);
  write_register(0x00e5, 0x02);
  write_register(0x00e6, 0x01);
  write_register(0x00e7, 0x03);
  write_register(0x00f5, 0x02);
  write_register(0x00d9, 0x05);
  write_register(0x00db, 0xce);
  write_register(0x00dc, 0x03);
  write_register(0x00dd, 0xf8);
  write_register(0x009f, 0x00);
  write_register(0x00a3, 0x3c);
  write_register(0x00b7, 0x00);
  write_register(0x00bb, 0x3c);
  write_register(0x00b2, 0x09);
  write_register(0x00ca, 0x09);
  write_register(0x0198, 0x01);
  write_register(0x01b0, 0x17);
  write_register(0x01ad, 0x00);
  write_register(0x00ff, 0x05);
  write_register(0x0100, 0x05);
  write_register(0x0199, 0x05);
  write_register(0x01a6, 0x1b);
  write_register(0x01ac, 0x3e);
  write_register(0x01a7, 0x1f);
  write_register(0x0030, 0x00);
  
  write_register(0x0011, 0x10);
  write_register(0x010a, 0x30);
  write_register(0x003f, 0x46);
  write_register(0x0031, 0xFF);
  write_register(0x0041, 0x63);
  write_register(0x002e, 0x01);
  write_register(0x001b, 0x09);
  write_register(0x003e, 0x31);
  write_register(0x0014, 0x24);   
  return 0;
}
#endif
int Vl6180xManager::OpenVl6180Normal()
{
    vl6180_fd=-1;
    if(access(vlNormalPath,F_OK)!=0)
    {
        DLOG_ERROR(MOD_EB,"stmvl6180_ranging device is not exist!\n"); 
        return -1;
    }
    vl6180_fd=open(vlNormalPath,O_RDWR|O_SYNC|O_NONBLOCK);
    if (vl6180_fd <= 0)
    {
        close(vl6180_fd);
        DLOG_ERROR(MOD_EB,"Error open stmvl6180_ranging device\n");
        return -1;
    }
        //make sure it's not started
    if (ioctl(vl6180_fd, VL6180_IOCTL_STOP , NULL) < 0) {
        DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_STOP\n");
        close(vl6180_fd);
        return -1;
    }	
    if (ioctl(vl6180_fd, VL6180_IOCTL_STOP_ALS , NULL) < 0) {
        DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_STOP\n");
        close(vl6180_fd);
        return -1;
    } 
    return 0;   
}

int Vl6180xManager::OpenVl6180Event()
{
    vl6180_fde=-1;
    vl6180_fde=open(VlInputPath,O_RDWR|O_SYNC|O_NONBLOCK);
    if (vl6180_fde <= 0)
    {
        DLOG_ERROR(MOD_EB,"Error open stmvl6180_ranging device\n");
        return -1;
    }
        //make sure it's not started
    if (ioctl(vl6180_fde, VL6180_IOCTL_STOP , NULL) < 0) {
        DLOG_ERROR(MOD_EB,"Error: Could not perform VL6180_IOCTL_STOP \n");
        close(vl6180_fde);
        return -1;
    }	
    if (ioctl(vl6180_fde, VL6180_IOCTL_STOP_ALS , NULL) < 0) {
        DLOG_ERROR(MOD_EB,"Error: Could not perform VL6180_IOCTL_STOP \n");
        close(vl6180_fde);
        return -1;
    } 
    return 0;   
}
int Vl6180xManager::InitVl6180Dev()
{
    if (ioctl(vl6180_fd, VL6180_IOCTL_INIT , NULL) < 0) {
        DLOG_ERROR(MOD_EB,"Error: Could not perform VL6180_IOCTL_INIT \n");
        close(vl6180_fd);
        return -1;
    }
    if (ioctl(vl6180_fd, VL6180_IOCTL_INIT_ALS , NULL) < 0) {
        DLOG_ERROR(MOD_EB,"Error: Could not perform VL6180_IOCTL_INIT_ALS \n");
        close(vl6180_fd);
        return -1;
    }
    VlWk_State=VL_WORK;
    return 0;
}

int Vl6180xManager::DeInitVl6180Dev()
{
    if (ioctl(vl6180_fd, VL6180_IOCTL_STOP , NULL) < 0) {
        DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_STOP : %s\n", strerror(errno));
        close(vl6180_fd);
        return -1;
    }	
    if (ioctl(vl6180_fd, VL6180_IOCTL_STOP_ALS , NULL) < 0) {
        DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_STOP : %s\n", strerror(errno));
        close(vl6180_fd);
        return -1;
    }  
    close(vl6180_fd); 
    return 0; 
}

int Vl6180xManager::Vl6180_Xtakcalib(Json::Value &jreq, Json::Value &jret)
{
    int num_samples = 20;
    int i=0;
    int RangeSum =0;
    int RateSum = 0;
    int XtalkInt =0;
    DLOG_INFO(MOD_EB, "xtalk Calibrate place black target at 100mm from glass===\n");
    // to xtalk calibration 
    if (ioctl(vl6180_fd, VL6180_IOCTL_XTALKCALB , NULL) < 0) {
        DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_XTALKCALB : %s\n", strerror(errno));
        VlWk_State=VL_SOFTRESET;   
        return -1;
    }
    for (i=0; i< num_samples; i++)
    {
        usleep(50*1000); /*50ms*/
        ioctl(vl6180_fd, VL6180_IOCTL_GETDATAS,&range_datas);
        DLOG_ERROR(MOD_EB," VL6180 Range Data:%d, error status:0x%x, Rtn Signal Rate:%d, signalRate_mcps:%d\n",
            range_datas.range_mm, range_datas.errorStatus, range_datas.rtnRate, range_datas.signalRate_mcps);	
        RangeSum += range_datas.range_mm;
        RateSum += 	range_datas.signalRate_mcps;// signalRate_mcps in 9.7 format

    }
    XtalkInt = (int)((float)RateSum/num_samples *(1-(float)RangeSum/num_samples/100));
    DLOG_INFO(MOD_EB, "VL6180 Xtalk Calibration get Xtalk Compensation rate in 9.7 format as %d\n",XtalkInt);
    if (ioctl(vl6180_fd, VL6180_IOCTL_SETXTALK,&XtalkInt) < 0) {
        DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_SETXTALK : %s\n", strerror(errno));
        VlWk_State=VL_SOFTRESET; 
        return -1;
    }
    //to stop
    //close(vl6180_fd);
    return 0;
}

int Vl6180xManager::Vl6180_Offcalib(Json::Value &jreq, Json::Value &jret)
{
    int num_samples = 20;
    int i=0;
    int RangeSum =0,RangeAvg=0;
    DLOG_INFO(MOD_EB, "offset Calibrate place white target at 50mm from glass===\n");
    // to offset calibration 
    if (ioctl(vl6180_fd, VL6180_IOCTL_INIT , NULL) < 0) {
        DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_INIT : %s\n", strerror(errno));
        VlWk_State=VL_SOFTRESET; 
        return -1;
    }
    for (i=0; i< num_samples; i++)
    {
        usleep(50*1000); /*50ms*/
        ioctl(vl6180_fd, VL6180_IOCTL_GETDATAS,&range_datas);
        DLOG_INFO(MOD_EB," VL6180 Range Data:%d, error status:0x%x, Rtn Signal Rate:%d, signalRate_mcps:%d\n",
            range_datas.range_mm, range_datas.errorStatus, range_datas.rtnRate, range_datas.signalRate_mcps);	
        RangeSum += range_datas.range_mm;

    }
    RangeAvg = RangeSum/num_samples;
    fprintf(stderr, "VL6180 Offset Calibration get the average Range as %d\n", RangeAvg);
    if ((RangeAvg >= (50-3)) && (RangeAvg <= (50+3)))
        fprintf(stderr, "VL6180 Offset Calibration: original offset is OK, finish offset calibration\n");
    else
    {
        int8_t offset=0;
        if (ioctl(vl6180_fd, VL6180_IOCTL_STOP , NULL) < 0) {
            DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_STOP : %s\n", strerror(errno));
            close(vl6180_fd);
            VlWk_State=VL_SOFTRESET; 
            return -1;
        }
        if (ioctl(vl6180_fd, VL6180_IOCTL_OFFCALB , NULL) < 0) {
            DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_OFFCALB : %s\n", strerror(errno));
            close(vl6180_fd);
            VlWk_State=VL_SOFTRESET; 
            return -1;
        }
        RangeSum = 0;
        for (i=0; i< num_samples; i++)
        {
            usleep(50*1000); /*50ms*/
            ioctl(vl6180_fd, VL6180_IOCTL_GETDATAS,&range_datas);
            fprintf(stderr," VL6180 Range Data:%d, error status:0x%x, Rtn Signal Rate:%d, signalRate_mcps:%d\n",
                range_datas.range_mm, range_datas.errorStatus, range_datas.rtnRate, range_datas.signalRate_mcps);	
            RangeSum += range_datas.range_mm;

        }
        RangeAvg = RangeSum/num_samples;
        fprintf(stderr, "VL6180 Offset Calibration get the average Range as %d\n", RangeAvg);
        offset = 50 - RangeAvg;
        fprintf(stderr, "VL6180 Offset Calibration to set the offset value(pre-scaling) as %d\n",offset);
        /**now need to resset the driver state to scaling mode that is being turn off by IOCTL_OFFCALB**/ 
        if (ioctl(vl6180_fd, VL6180_IOCTL_STOP , NULL) < 0) {
            DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_STOP : %s\n", strerror(errno));
            close(vl6180_fd);
            VlWk_State=VL_SOFTRESET; 
            return -1;
        }	
        if (ioctl(vl6180_fd, VL6180_IOCTL_INIT , NULL) < 0) {
            DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_INIT : %s\n", strerror(errno));
            close(vl6180_fd);
            VlWk_State=VL_SOFTRESET; 
            return -1;
        }
        if (ioctl(vl6180_fd, VL6180_IOCTL_SETOFFSET, &offset) < 0) {
            DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_SETOFFSET : %s\n", strerror(errno));
            close(vl6180_fd);
            VlWk_State=VL_SOFTRESET; 
            return -1;
        }
    }
    close(vl6180_fd);
    return 0;
}
bool Vl6180xManager::Vl6180_Range()
{
   // usleep(10 *1000); /*10ms*/
     // to get all range data
    m_tex.lock();	
    if(ioctl(vl6180_fd, VL6180_IOCTL_GETDATAS,&range_datas)<0)
    {
        DLOG_ERROR(MOD_EB,"Error: Could not perform VL6180_IOCTL_GETDATAS : %s\n", strerror(errno));
        VlWk_State=VL_SOFTRESET; 
        m_tex.unlock();
        return false;
    }
    vl6180_dis=range_datas.range_mm;
    m_tex.unlock();
    //fprintf(stderr," VL6180 Range Data:%d, error status:0x%x, Rtn Signal Rate:%d, signalRate_mcps:%d, Rtn 	Amb Rate:%d\n",	range_datas.range_mm,range_datas.errorStatus,range_datas.rtnRate,range_datas.signalRate_mcps, range_datas.rtnAmbRate);
    return true;
}
bool Vl6180xManager::Read_Als()
{
    // get data testing
    //usleep(30 *1000); /*30ms*/
        // to get lux value
    if(ioctl(vl6180_fd, VL6180_IOCTL_GETLUX,&lux_data)<0)
    {
        DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_GETLUX : %s\n", strerror(errno));
        VlWk_State=VL_SOFTRESET; 
        return false;
    }
    fprintf(stderr," VL6180 Lux Value:%ld\n",lux_data);
    return true; 
}
bool Vl6180xManager::Read_SingleData()
{
    // get data testing       
   // usleep(100 *1000); /*10ms*/
    // to get all range data
    m_tex.lock();
    if(ioctl(vl6180_fd, VL6180_IOCTL_GETDATA,&range_datas.range_mm)<0)
    {
        DLOG_ERROR(MOD_EB, "Error: Could not perform VL6180_IOCTL_GETDATA : %s\n", strerror(errno));
        VlWk_State=VL_SOFTRESET; 
        m_tex.unlock();
        return false;       
    }
    vl6180_dis=range_datas.range_mm;
    m_tex.unlock();
  //  DLOG_INFO(MOD_EB," VL6180 Range Data:%d\n",range_datas.range_mm);
    return true;
}
bool Vl6180xManager::Read_EventData()
{
    int err;
    err=read(vl6180_fde,&inputevent,sizeof(inputevent));
    if(err>0)
    {
        printf(" read file %s\r\n",VlInputPath);
        switch (inputevent.type)
        {
        case EV_ABS:
            if(inputevent.code==ABS_HAT1X)
            {
                range_datas.range_mm=inputevent.value;
               // fprintf(stderr," EventCode=%d,VL6180 Range Data:%d\n",inputevent.code,range_datas.range_mm);
            }
            else{
                fprintf(stderr," EventCode=%d\n",inputevent.code);
            }      
            /* code */
            break;  
        default:
            break;
        }
    }else{
        VlWk_State=VL_SOFTRESET; 
        printf("read error\r\n");
        return false;
    }
    return true;
}
bool Vl6180xManager::Vl6180_Range_Als()
{   
    // get data testing
   // usleep(30 *1000); /*100ms*/
    // to get proximity value only
    /*
    ioctl(vl6180_fd, VL6180_IOCTL_GETDATA , &data) ;
    */
    // to get all range data
    if(ioctl(vl6180_fd, VL6180_IOCTL_GETDATAS,&range_datas)<0)
    {
        fprintf(stderr, "Error: Could not perform VL6180_IOCTL_GETDATA : %s\n", strerror(errno));
        VlWk_State=VL_SOFTRESET; 
        return false;    
    }
    else{
        // fprintf(stderr," VL6180 Range Data:%d, error status:0x%x, Rtn Signal Rate:%d, signalRate_mcps:%d, Rtn Amb Rate:%d\n",
        // range_datas.range_mm, range_datas.errorStatus, range_datas.rtnRate,range_datas.signalRate_mcps, range_datas.rtnAmbRate);
    }  
    // to get lux value
    if(ioctl(vl6180_fd, VL6180_IOCTL_GETLUX,&lux_data)<0)
    {
        fprintf(stderr, "Error: Could not perform VL6180_IOCTL_GETDATAS : %s\n", strerror(errno));
        VlWk_State=VL_SOFTRESET; 
        return false;         
    }
    else{
        fprintf(stderr," VL6180 Lux Value:%ld\n",lux_data);
    }
    return true;
}
#if USE_I2C2_PORT
int Vl6180xManager::Vl6180_Run()
{ 
  if (!connected) return 0xFF;
  if (!is_measuring) {
    uint8_t status = read_register(REG_RESULT_RANGE_STATUS);
    // Read range status register.
    if ((status & 0x01) == 0x01) {
      // Start a range measurement.
      write_register(REG_SYSRANGE_START, 0x01);
    
      is_measuring = true;
    }
    else {
      std::cout << "VL6180X_ToF 0x4d Status Error " << status << '\n';
    }
  }

  if (is_measuring) {
    if ((read_register(REG_RESULT_INTERRUPT_STATUS_GPIO) & 0x04) == 0x04) {
      // Read range register (mm).
      range = read_register(REG_RESULT_RANGE_VAL);
      printf("range = %d\n",range);
      vl6180_dis=range;
      // Clear the interrupt.
      write_register(REG_SYSTEM_INTERRUPT_CLEAR, 0x07);
    
      is_measuring = false;
    }
  }
  return 0; 
}
#endif
int Vl6180xManager::Vl6180_Run()
{
    switch (VlWk_State)
    {
    case VL_CLOSE:
        /* code */
        DeInitVl6180Dev();
        VlWk_State=VL_OPEN;
       // printf("VlWk_State is VL_CLOSE\n");
        break;
    case VL_OPEN:
        OpenVl6180Normal();
       // OpenVl6180Event();
        VlWk_State=VL_INIT;
        //printf("VlWk_State is VL_OPEN\n");
        break;
    case VL_INIT:
        if(InitVl6180Dev()<0) //error
        {
            VlWk_State=VL_SOFTRESET;  
        }
        else {
            VlWk_State=VL_WORK;
        }
        //printf("VlWk_State is VL_INIT\n");
        break;
    case VL_SOFTRESET:
        reset_num++;
        if(reset_num>=10)
        {
            VlWk_State=VL_HWRESET;
            reset_num=0;
        }
        else  VlWk_State=VL_CLOSE;
        // printf("VlWk_State is VL_SOFTRESET\n");
        break;
    case VL_HWRESET: 
        DeInitVl6180Dev(); //先close设备操作符，再关闭电源
        EDGE_PWR_OFF;
        usleep(500*1000);
        EDGE_PWR_ON;
        VlWk_State=VL_OPEN;
        // printf("VlWk_State is VL_HWRESET\n");
        break;
    case VL_WORK:
        vl6180_Sample();
        // printf("VlWk_State is VL_WORK\n");
        break;
    default:
        break;
    }
    return 0;
}
void Vl6180xManager::vl6180_Sample()
{

    switch (SamState)
    {
        case MODE_RANGE:
            SampleDone=Vl6180_Range();
            //usleep(30 *1000);
           //printf("SamState is MODE_RANGE\n");
            break;
        case MODE_ALS:
            SampleDone=Read_Als();
            //usleep(30 *1000);
            //printf("SamState is MODE_ALS\n");
            break;
        case MODE_RANGE_ALS: //读数据和als
            SampleDone=Vl6180_Range_Als();
            //usleep(30 *1000);
           // printf("SamState is MODE_RANGE_ALS\n");
            break;
        case SINGLE_DATA:
            SampleDone=Read_SingleData();
            // usleep(100 *1000);
          
           // dp_client_->SendDpMessage("DP_PERIPHERALS_BROADCAST", 0, str_req.c_str(), str_req.size(), NULL);
           // DLOG_DEBUG(MOD_EB, "range_datas is %d\n",range_datas.range_mm);
           // printf("SamState is SINGLE_DATA\n");
            break;
        case EVENT_DATA:
            SampleDone=Read_EventData();
            //usleep(100 *1000);
           // printf("SamState is EVENT_DATA\n");
            break;
        case IDLE:
            usleep(100*1000);
           // printf("SamState is IDLE\n");
        default:
            break;
    }
}
#if USE_I2C2_PORT
uint8_t Vl6180xManager::read_register(uint16_t reg) {
  if (!connected) return 0;
  
  uint8_t buffer[3];
  buffer[0] = uint8_t(reg >> 8);
  buffer[1] = uint8_t(reg & 0xFF);
  buffer[2] = 0;
  
  // Prompt the sensor with the address to read.
  write(i2c_fd, buffer, 2);
  
  // Read the value at the address.
  read(i2c_fd, &buffer[2], 1);
  return buffer[2];
}

void Vl6180xManager::write_register(uint16_t reg, uint8_t data) {
  if (!connected) return;
  
  uint8_t buffer[3];
  
  buffer[0] = uint8_t(reg >> 8);
  buffer[1] = uint8_t(reg & 0xFF);
  buffer[2] = data;
  
  // Write data to register at address.
  write(i2c_fd, buffer, 3);
}
#endif
    
}