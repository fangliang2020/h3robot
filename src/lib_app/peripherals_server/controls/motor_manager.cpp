#include "motor_manager.h"
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "peripherals_server/main/device_manager.h"

uint16_t fanspeed[4]= {5,10,35,45};
uint16_t midspeed[3]= {9,12,14};
uint16_t side_speed[3]= {4,6,8};
uint16_t mop_speed[3]={42000,30000,25000};
const uint8_t startdata[2]={0x02,0xFD};
#define MSG_MOTOR_TIMING   0x101
#define MSG_MOPRISE_DELAY  0x107

uint8_t  CommParam::rug=0;

namespace peripherals{

DP_MSG_HDR(MotorManager,DP_MSG_DEAL,pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[]={
    {"set_fan_level",   &MotorManager::Set_Fan_Lev},
    {"set_mid_level",   &MotorManager::Set_Mid_Lev},
    {"set_side_level",  &MotorManager::Set_Side_Lev},
    {"set_mop_ctrl",     &MotorManager::Set_Mop_Lev}
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

MotorManager::MotorManager(PerServer *per_server_ptr)
    : BaseModule(per_server_ptr) {	
    Event_Service_=Perptr_->GetEventServicePtr();
}
MotorManager::~MotorManager() {

}
/****************fan_motor dev ctrl 风机设备管理*****************/
int MotorManager::Start() {
    fan_fd=-1;
    mid_fd=-1;
    side_fd=-1;
    mop_fd=-1;
    ultra_fd = -1;
    int ret=0;
    /*初始化风机设备节点*/
    DLOG_DEBUG(MOD_EB, "init fan motor\n");
    if(access(DEVFAN_PATH,F_OK)!=0)
    {
        DLOG_ERROR(MOD_EB, "fan dev is not exist!\n");
        return -1;
    }
    fan_fd=open(DEVFAN_PATH,O_RDWR);
    if(fan_fd<0)
    {
        close(fan_fd);
        DLOG_ERROR(MOD_EB, "open %s dev error!\n",DEVFAN_PATH);
        return -1;
    }
    ioctl(fan_fd,FAN_CTL_ENABLE,1);
    Motorfan.workstate=0;
    /*初始化中扫设备节点*/
    DLOG_DEBUG(MOD_EB, "init mid motor\n");
    if(access(DEVMID_PATH,F_OK)!=0)
    {
        DLOG_ERROR(MOD_EB, "mid dev is not exist!\n");
        return -1;
    }
    mid_fd=open(DEVMID_PATH,O_RDWR);
    if(mid_fd<0)
    {
        close(mid_fd);
        DLOG_ERROR(MOD_EB, "open %s dev error!\n",DEVMID_PATH);
        return -1;
    }
    ioctl(mid_fd,PWM_MID_ENABLE,1);
    /*初始化边扫设备节点*/
    DLOG_DEBUG(MOD_EB, "init side motor\n");
    if(access(DEVSIDE_PATH,F_OK)!=0)
    {
        DLOG_ERROR(MOD_EB, "side dev is not exist!\n");
        return -1;
    }
    side_fd=open(DEVSIDE_PATH,O_RDWR);
    if(side_fd<0)
    {
        close(side_fd);
        DLOG_ERROR(MOD_EB, "open %s dev error!\n",DEVSIDE_PATH);
        return -1;
    }
    ioctl(side_fd,PWM_SIDE_ENABLE,1);
    /*初始化拖布设备节点*/
    DLOG_DEBUG(MOD_EB, "init mop motor\n");
    if(access(DEVMOP_PATH,F_OK)!=0)
    {
        DLOG_ERROR(MOD_EB, "mop dev is not exist!\n");
        return -1;
    }
    mop_fd=open(DEVMOP_PATH,O_RDWR);
    if(mop_fd<0)
    {
        close(mop_fd);
        DLOG_ERROR(MOD_EB, "open %s dev error!\n",DEVMOP_PATH);
        return -1;
    }
    /*初始化超声波设备节点*/
    DLOG_DEBUG(MOD_EB, "init ultra_dev\n");
    if(access(ULTRA_PATH,F_OK)!=0)
    {
        DLOG_ERROR(MOD_EB, "mop dev is not exist!\n");
        return -1;
    }
    ultra_fd=open(ULTRA_PATH,O_RDWR);
    if(ultra_fd<0)
    {
        close(ultra_fd);
        DLOG_ERROR(MOD_EB, "open %s dev error!\n",ULTRA_PATH);
        return -1;
    }
    ioctl(ultra_fd,I2C_SLAVE,0x51);
    ret = write(ultra_fd,startdata,sizeof(startdata));
    if(ret<0) {
        DLOG_ERROR(MOD_EB,"ultra_fd write error\n");
    }
    //ioctl(mop_fd,MOP_CTL_ENABLE,1);  
    Event_Service_->PostDelayed(50, this, MSG_MOTOR_TIMING);
    //初始化正确返回0  
    return 0;
}
int MotorManager::Stop()
{
    //关闭风机节点
    DLOG_DEBUG(MOD_EB, "close fan motor\n");
    ioctl(fan_fd,FAN_CTL_DISABLE,1);
    close(fan_fd);
    //关闭中扫节点
    DLOG_DEBUG(MOD_EB, "close mid motor\n");
    ioctl(mid_fd,PWM_MID_DISABLE,1);
    close(mid_fd);
    //关闭边扫节点
    DLOG_DEBUG(MOD_EB, "close side motor\n");
    ioctl(side_fd,PWM_SIDE_DISABLE,1);
    close(side_fd);
    //关闭拖布节点
    DLOG_DEBUG(MOD_EB, "close mop motor\n");
    ioctl(mop_fd,MOP_CTL_DISABLE,1);
    close(mop_fd);
    close(ultra_fd);
    return 0;
}

int MotorManager::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
    DLOG_INFO(MOD_EB, "OnDpMessage:%s.", stype.c_str());
    int nret = -1;
    for (uint32 idx = 0; idx < g_dpmsg_table_size; ++idx) {
        if (stype == g_dpmsg_table[idx].type) {
        nret = (this->*g_dpmsg_table[idx].handler) (jreq["body"], jret);
        break;
        }
    }

    return nret;
}

void MotorManager::OnMessage(chml::Message* msg) {
    if(MSG_MOTOR_TIMING == msg->message_id) {
        Timer50ms();
        Event_Service_->PostDelayed(50,this,MSG_MOTOR_TIMING);
        // Event_Service_->Clear(this,MSG_MOTOR_TIMING); 停止定时器。
    }
    if(MSG_MOPRISE_DELAY == msg->message_id) {
        ioctl(mop_fd, MOP_CTL_DISABLE,0); 
        system("echo 0 >  /sys/devices/platform/pc-switch/pc_switch/switch/do_clean_pump");
        printf("MOP_CTL_DISABLE\n");
    }
}

void MotorManager::Timer50ms() {
    Fan_Motor_Run();
    Mid_Motor_Run();
    Side_Motor_Run();
    Mop_Motor_Run();
    Ultra_Sample();
}

int MotorManager::Set_Fan_Lev(Json::Value &jreq, Json::Value &jret)
{
    int JV_TO_INT(jreq,"level",level,0);
    if(level==0) {
        FAN_PWR_OFF;
        Motorfan.workstate=0;
        Motorfan.setspeed=0;
		Motorfan.readspeed=0;
        ioctl(fan_fd, FAN_CTL_DISABLE,0);
		ioctl(fan_fd, FAN_CTL_SET_DUTY,&Motorfan.setspeed);
		MotorPidclear(&fMotorPid);  
    }
    else if(level>0) {
       //FAN_PWR_ON;
       ioctl(fan_fd, FAN_CTL_ENABLE,0);
       Motorfan.workstate=level;
       Motorfan.setspeed=fanspeed[Motorfan.workstate-1];
    }
    DLOG_INFO(MOD_EB, "Set_Fan_Lev:%d\n",level);
    return 0;
}

void MotorManager::Fan_Motor_Run()
{
    
    uint32_t Fan_Out_Pwm=0;
	if(Motorfan.workstate)
	{	
      //  printf("fan workstate=%d\n",Motorfan.workstate);
		if(Motorfan.M_Prot.status==1)//开路，过流保护
		{
			ioctl(fan_fd, FAN_CTL_SET_DUTY,&Motorfan.setspeed);
			MotorPidclear(&fMotorPid);
		}else{
			//FAN_PWR_ON;
			//Motorfan.setspeed=fanspeed[Motorfan.workstate-1];
 
			Fan_Out_Pwm=PidFun_Fan(&Motorfan);// 只有一档最大速度
            //Fan_Out_Pwm = 10000*Motorfan.workstate;
			/*****更新PWM的脉宽********/
			ioctl(fan_fd, FAN_CTL_SET_DUTY,&Fan_Out_Pwm);

            //ioctl(fan_fd,FAN_CTL_GET_SPEED,&Motorfan.readspeed);
            // printf("read fan speed=%d\n", Motorfan.readspeed);
            // printf("set fan speed=%d\n", Fan_Out_Pwm);
		}
	}
}

/****************mid_motor dev ctrl 中扫设备管理*****************/

int MotorManager::Set_Mid_Lev(Json::Value &jreq, Json::Value &jret)
{
    int JV_TO_INT(jreq,"level",level,0);
    if(level==0) {
        Motormid.workstate=0;
        Motormid.setspeed=0;
		Motormid.readspeed=0;
		ioctl(mid_fd, FAN_CTL_SET_DUTY,&Motormid.setspeed); 
    }
    else if(level>0) {
        Motormid.workstate=level;
    }
    DLOG_INFO(MOD_EB, "Set_Mid_Lev:%d\n",level);
    return 0;
}

void MotorManager::Mid_Motor_Run()
{
    if(Motormid.workstate) {
        if(Motormid.M_Prot.status==1)//开路过流保护
        {
            Motormid.setspeed=0;
            Motormid.readspeed=0;
            ioctl(mid_fd,PWM_MID_SET_DUTY,&Motormid.setspeed);
        }
        else{
            Motormid.setspeed=midspeed[Motormid.workstate-1]*66667/16;
            //  Motormid.setspeed=midspeed[Motormid.workstate-1]*7200/VBAT_Prot.currentValue;//根据当前电压算出PWM          
            if(Motormid.setspeed>=66667) Motormid.setspeed=66667; 
            ioctl(mid_fd,PWM_MID_SET_DUTY,&Motormid.setspeed);
        }
    }
}

/****************side_motor dev ctrl 边扫设备管理*****************/
int MotorManager::Set_Side_Lev(Json::Value &jreq, Json::Value &jret)
{
    int JV_TO_INT(jreq,"level",level,0);
    if(level==0) {
        MotorsideA.workstate=0;
        MotorsideA.setspeed=0;
		MotorsideA.readspeed=0;
        MotorsideB.workstate=0;
        MotorsideB.setspeed=0;
		MotorsideB.readspeed=0;
		ioctl(side_fd, PWM_SIDE_SET_DUTY,&MotorsideA.setspeed); 
    }
    else if(level>0) {
        MotorsideA.workstate=level;
        MotorsideB.workstate=level;
    }
    DLOG_INFO(MOD_EB, "Set_Side_Lev:%d\n",level);
    return 0;
}

void MotorManager::Side_Motor_Run()
{
    if(MotorsideA.workstate||MotorsideB.workstate) {
        if((MotorsideA.M_Prot.status==1)||(MotorsideB.M_Prot.status==1))//开路过流保护
        {
            ioctl(side_fd,PWM_SIDE_SET_DUTY,0);
        }
        else{
//          Motorside.setspeed=midspeed[Motorside.workstate-1]*7200/VBAT_Prot.currentValue;//根据当前电压算出PWM
           MotorsideA.setspeed=side_speed[MotorsideA.workstate-1]*66667/16;
           if(MotorsideA.setspeed>=66667) MotorsideA.setspeed=66667; 
           ioctl(side_fd,PWM_SIDE_SET_DUTY,&MotorsideA.setspeed);
        }
    }
}
/****************mop_motor dev ctrl 拖布电机设备管理
 *  测试版本： 收到0 拖布抬升并停止
 *            收到1 拖布放下并旋转
 * *****************/

int MotorManager::Set_Mop_Lev(Json::Value &jreq, Json::Value &jret)
{
    int JV_TO_INT(jreq,"ctrl",ctrl,0);
    if(ctrl==0) {  //抬升
        MotorMop.workstate=0;
        MotorMop.setspeed=30000;
		MotorMop.readspeed=0;
        MOP_UP;
        ioctl(mop_fd, MOP_CTL_SET_DUTY,&MotorMop.setspeed);
        Event_Service_->PostDelayed(3000, this, MSG_MOPRISE_DELAY); //拖布抬升延迟后关闭
		//ioctl(mop_fd, MOP_CTL_DISABLE,0); 
    }
    else if(ctrl>0) { 
        ioctl(mop_fd, MOP_CTL_ENABLE,0); 
        MotorMop.workstate=ctrl;
        MOP_DOWN;//下降
        MotorMop.setspeed=mop_speed[MotorMop.workstate-1];
        ioctl(mop_fd,MOP_CTL_SET_DUTY,&MotorMop.setspeed);
    }
    DLOG_INFO(MOD_EB, "Set_Mop_Lev:%d\n",ctrl);
    return 0;
} 

void MotorManager::Mop_Motor_Run()
{
    //uint32_t Mop_Out_Pwm=0;
	if(MotorMop.workstate) {	
      //  printf("fan workstate=%d\n",Motorfan.workstate);
		if(MotorMop.M_Prot.status==1)//开路，过流保护
		{
			ioctl(mop_fd, MOP_CTL_SET_DUTY,0);
            system("echo 0 >  /sys/devices/platform/pc-switch/pc_switch/switch/do_clean_pump");
		}
        else {
			/*****更新PWM的脉宽********/
			//ioctl(mop_fd, MOP_CTL_SET_DUTY,&MotorMop.setspeed);
            /* 喷水处理*/   
            mop_cnt++;
            if(mop_cnt==15){
                system("echo 1 >  /sys/devices/platform/pc-switch/pc_switch/switch/do_clean_pump");
            }
            else if(mop_cnt>=20){
                mop_cnt=0;
                system("echo 0 >  /sys/devices/platform/pc-switch/pc_switch/switch/do_clean_pump");
            }

		}
	}
}

void MotorManager::Ultra_Sample()
{
    read(ultra_fd,&ultra_buf[0],13);
    if(ultra_buf[0]==0) {
        rug=0;
    }  
    else {
        rug=1;
    }
}
}