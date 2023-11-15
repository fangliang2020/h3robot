#include "RobotSchedule.h"
#include "DateModel.h"
#include "robot_schedule/clean/control_interface.hpp"
#include "Timer.h"
#include <unistd.h> 
#include <numeric>
#include <iostream>
#include <string>
#include <queue> //队列头文件
#include "NetserverInstruct.h"
//#include"cJSON.h"
//#include <cjson/cJSON.h>

#include "robot_schedule/clean/config.hpp"
#include "robot_schedule/socket/navigate.h"
//#include "Peripheral/device_manager.h"

using namespace std;
#define STATE_DATA 2
#define JOBCMD_DATA 2
#define START_CMD 1
#define SAVE_MAP 'S'
#define GET_MAP 'G'
//bool SIG_VALUE_SCHEDULE = true;
std::mutex CRobotSchedule::m_scheduleMutex;
CRobotSchedule *CRobotSchedule::m_RobotSchedule = nullptr;
CRobotSchedule::CRobotSchedule()
{
    printf("init ROBOT thread\n");
    m_cleanTh = nullptr;
    m_saveMapTh = nullptr;
    m_timerTh = nullptr;
    clean_return_value = 2;
    
 
    // dp_state_client_ = nullptr;
    // Mystate_Service_ = nullptr;
 
}
CRobotSchedule::~CRobotSchedule()
{
    m_schedule = false;
    if (m_cleanTh)
    {
        
        m_cleanTh->join();
        delete(m_cleanTh);
        std::cout << "m_cleanTH delete" << std::endl;
    }
    if (m_saveMapTh)
    {
        m_saveMapTh->join();
        delete(m_saveMapTh);
        std::cout << "m_saveMapTh delete" << std::endl;
    }
    //if (m_listenTh)
    //{
      //  m_listenTh->join();
        //delete(m_listenTh);
    //}
    if(m_timerTh)
	{
        m_timerTh->join();
		delete(m_timerTh);
        std::cout << "m_timerTh delete" << std::endl;
	}
    printf("deleat all thread\n");
    
}

CRobotjobEvent::CRobotjobEvent()
{
    job_cmd = EJobCommand(EJOB_NONE);
}
CRobotjobEvent::~CRobotjobEvent()
{
}
void CRobotjobEvent::deljobEvent()
{
    if (m_cleanstance)
    {
        delete(m_cleanstance);
        m_cleanstance = nullptr;
    }
    std::cout << "m_cleanstance delete" << std::endl;
}
CRobotjobEvent *CRobotjobEvent::m_cleanstance = nullptr;
std::mutex CRobotjobEvent::m_jobMutex;
CRobotjobEvent *CRobotjobEvent::getjobEvent()
{
    if (m_cleanstance == nullptr)
    {
        m_jobMutex.lock();
        if (m_cleanstance == nullptr)
        {
            m_cleanstance = new CRobotjobEvent();
        }
        m_jobMutex.unlock();
        
    }

    return m_cleanstance;
}
void CRobotSchedule::setCRobotdata(bool m_sigvalue)
{
    SIG_VALUE_SCHEDULE = m_sigvalue;
}
bool CRobotSchedule::getCRobotdata()
{
    return SIG_VALUE_SCHEDULE;
}
int CRobotSchedule::getcurrentmap(int mapID)
{
   if (mapID == 1)
   {
        mapPatchname = "/userdata/map/onemap.map";
   }
   if (mapID == 2)
   {
        mapPatchname = "/userdata/map/twomap.map";
   }
   if (mapID == 3)
   {
        mapPatchname = "/userdata/map/lastmap.map";
   }
   
    auto mapFD = fopen(mapPatchname.c_str(),"r");
    if (!mapFD)
    {
        std::cout << "open slammap file failed" << std::endl;
        return -1;
    }
    fseek(mapFD, 0, SEEK_END);
    size_t size = ftell(mapFD);
    int8_t* where = new int8_t[size];
    rewind(mapFD);
    fread(where, sizeof(int8_t), size, mapFD);
    printf("where=%s\n", where);
    fclose(mapFD);
}
void CRobotSchedule::startSlam()
{
    //m_slamState = SLAM_STARTING;
    /*code 算法去添加调用算法的启动接口如果有地图直接加载地图，没有重新建图*/

    /**??????????**/
    //LOG4_DEBUG("startSlamModule end");
    //m_slamState = SLAM_STARTED;
}
void CRobotSchedule::saveSlam_mapdata()
{
    //socket_client(SAVE_MAP);
    
}
void CRobotSchedule::getSlam_mapdata()
{
    //socket_client(GET_MAP);


}
void CRobotSchedule::socket_client(char ch)
{
    int sockfd = -1;  
    int len = 0;  
    struct sockaddr_in address;  
    int result;  
    //char ch = 'A';  
    //创建流套接字  
    sockfd = socket(AF_INET, SOCK_STREAM, 0);  
    //设置要连接的服务器的信息  
    address.sin_family = AF_INET;//使用网络套接字  
    address.sin_addr.s_addr = inet_addr("127.0.0.1");//服务器地址  
    address.sin_port = htons(9736);//服务器所监听的端口  
    len = sizeof(address);  
    //连接到服务器  
    result = connect(sockfd, (struct sockaddr*)&address, len);  
  
    if(result == -1)  
    {  
        perror("ops:client\n");  
        exit(1);  
    }  
    //发送请求给服务器  
    write(sockfd, &ch, 1);  
    //从服务器获取数据  
    //read(sockfd, &ch, 1);  
    printf("char form server = %c\n", ch);  
    close(sockfd);  
    exit(0); 

}
void CRobotSchedule::startSlamModule(bool initMap)
{
    /****获取slam状态接口??算法提供****/
    m_slamState = getSlamStartState();
    if (m_slamState == SLAM_IDLE)
    {
        m_saveMapTh = new std::thread(std::bind(&CRobotSchedule::startSlam, this));
    }
    if (m_slamState == SLAM_PAUSE)
    {
        /* code 继续运行SLAM 调用算法的接口*/
        /**?????????**/

        //m_slamState = SLAM_STARTED;
    }
}
int CRobotSchedule::getcleantype()
{
    return clean_type;
}
int CRobotSchedule::getcleanresult()
{
    return clean_result;
}
void CRobotSchedule::allWorkInterface(EJobCommand e_Cmd,E_MASTER_STATE e_Mstate,CLEAN_STATE e_Cstate,std::string play_voice)
{
    CDataModel::getDataModel()->getCurrentTime();
    CDataModel::getDataModel()->setMState(e_Mstate);
    CDataModel::getDataModel()->setCleanState(e_Cstate);
    // if (e_Cmd == EJOB_GLOBAL_CLEAN || e_Cmd == EJOB_LOCAL_CLEAN || e_Cmd == EJOB_ZONE_CLEAN || e_Cmd == EJOB_POINT_CLEAN)
    // {
    //     //DbdataBase::getDbInstruct()->work_clean_type();
    // }
    //DbdataBase::getDbInstruct()->save_db_map();
    SensorData::getsensorData()->set_voice_play(play_voice);
    CRobotSchedule::sendState();
    
    CRobotjobEvent::getjobEvent()->setJobCommand(EJOB_NULL);
}
void CRobotSchedule::startWorkSchedule(bool bLoadMap)
{
    // 启动slam
    startSlamModule(bLoadMap);
}
void CRobotSchedule::cleanCommand(int cmd)
{
    startWorkSchedule(); // 启动slam
    /*清扫前获取回洗面积*/
    
    /******************************/
    switch (cmd)
    {
    case EJOB_GLOBAL_CLEAN:
        
        allWorkInterface(EJOB_GLOBAL_CLEAN,ESTATE_CLEAN,CLEAN_STATE_AUTO,PLAY_Q006);
        clean_type = 1;
        break;
    case EJOB_LOCAL_CLEAN: //划区
        allWorkInterface(EJOB_LOCAL_CLEAN,ESTATE_CLEAN,CLEAN_STATE_RECT,PLAY_Q044);
        clean_type = 2;
        break; 
    case EJOB_ZONE_CLEAN:  //定点清扫,房间清扫
        allWorkInterface(EJOB_ZONE_CLEAN,ESTATE_CLEAN,CLEAN_STATE_LOCATION,PLAY_Q009);
        clean_type = 3;
        break;
    case EJOB_POINT_CLEAN: //指哪儿扫哪儿
        allWorkInterface(EJOB_POINT_CLEAN,ESTATE_CLEAN,CLEAN_STATE_SITU,PLAY_Q006);
        clean_type = 4;
        break;
    case EJOB_FAST_BUILD_MAP:
        allWorkInterface(EJOB_FAST_BUILD_MAP,ESTATE_FAST_BUILD_MAP,CLEAN_STATE_NULL,PLAY_Q006);

        break;
    case EJOB_BACK_CHARGE:
        allWorkInterface(EJOB_BACK_CHARGE,ESTATE_BACK_CHARGE,CLEAN_STATE_NULL,PLAY_Q006);
       
        break;

    default:
        break;
    }
    
}
void CRobotSchedule::newCommandHandle()
{

    auto jobcommand = CRobotjobEvent::getjobEvent()->getJobCommand();
    
    if (jobcommand >= EJOB_GLOBAL_CLEAN && jobcommand <= EJOB_VLOW_BACK_CHARGE)
    {
        cleanCommand(jobcommand);
    }
    
    usleep(10*1000);
}
void CRobotSchedule::cleanWorkHandle()
{
    auto mainState_a = CDataModel::getDataModel()->getMState();
    auto cleanState_a = CDataModel::getDataModel()->getCleanState();
    
    if (mainState_a == ESTATE_CLEAN)
    {
        
        switch (cleanState_a)
        {
            
            case CLEAN_STATE_AUTO:
                clean_return_value = house_clean(0,1,0);
                DbdataBase::getDbInstruct()->saveAllmaponDb();
                std::cout << "the house clean is exit" <<std::endl;
                break;
            case CLEAN_STATE_RECT:
                //area_clean();
                break;
            case CLEAN_STATE_LOCATION:
                room_clean(LIVING_ROOM,0);
                break;
            case CLEAN_STATE_SITU:
                break;
            
            default:
                break;
        }

    }
    
    
}
void CRobotSchedule::cleanSchedule()
{
    printf("cleanSchedule start\n");
    CRobotSchedule::sendState();
    while (true)
    {
        
        newCommandHandle();
        cleanWorkHandle();
        
        switch (clean_return_value)
        {
        case 0: 
            NetserverInstruct::getNetInstruct()->send_clean_log();
            clean_result = 1;
            break;
        case 1:
            NetserverInstruct::getNetInstruct()->send_clean_log();
            SensorData::getsensorData()->set_voice_play(PLAY_Q010);
            CDataModel::getDataModel()->setMState(ESTATE_BACK_CHARGE);
            sendState();
            clean_result = 2;
            clean_return_value = 2;
            break;
        default:
            break;
        }
        usleep(10*1000);
        if (getCRobotdata() == false)
        {
            break;
        }
        
    }
}

void CRobotSchedule::listenState()
{
    
    if (CDataModel::getDataModel()->getCQueueSize() == 2)
    {
        if (CDataModel::getDataModel()->getCQueueFort() != CDataModel::getDataModel()->getCQueueBack())
        {
            interrupt_work();
            CDataModel::getDataModel()->deletCQueue();
        }
        
        
    }
    if (CDataModel::getDataModel()->getMQueueSize() == 2)
    {
        int back_data = CDataModel::getDataModel()->getMQueueBack();
        if (CDataModel::getDataModel()->getMQueueFort() == ESTATE_CLEAN)
        {
            switch (back_data)
            {
            case ESTATE_BACK_CHARGE:
                interrupt_work();
                break;
            case ESTATE_BACK_WASH:
                interrupt_work();
                break;
            default:
                break;
            }
        }
        
    }
    
    
    
}
void CRobotSchedule::sendmapdata()
{
  
    auto now_Mstate = CDataModel::getDataModel()->getMState();
    
    if (now_Mstate == ESTATE_CLEAN )
    {
        chml::DpClient::Ptr netserver_client_ = NetserverInstruct::getNetInstruct()->GetDpnetserverClientPtr();
        std::string str_req, str_res;
        Json::Value json_req;
        struct map_update map_data;
        switch (map_data.map_id)
        {
        case 1:
        {
            json_req[JKEY_TYPE] = "update_current_map";
            json_req[JKEY_BODY]["map_id"] = 1;
            json_req[JKEY_BODY]["map_info_path"] = "/userdata/map/housemap_info.txt";
            json_req[JKEY_BODY]["map_data_path"] = "/userdata/map/housemap.txt";
            Json2String(json_req, str_req);
            netserver_client_->SendDpMessage(DP_SCHEDULE_BROADCAST, 0, str_req.c_str(), str_req.size(), &str_res);
            break;
        }
        case 2:
        {
            json_req[JKEY_TYPE] = "update_current_map";
            json_req[JKEY_BODY]["map_id"] = 2;
            json_req[JKEY_BODY]["map_info_path"] = "/userdata/map/housemap_info2.txt";
            json_req[JKEY_BODY]["map_data_path"] = "/userdata/map/housemap2.txt";
            Json2String(json_req, str_req);
            netserver_client_->SendDpMessage(DP_SCHEDULE_BROADCAST, 0, str_req.c_str(), str_req.size(), &str_res);
            break;
        }
        case 3:
        {
            json_req[JKEY_TYPE] = "update_current_map";
            json_req[JKEY_BODY]["map_id"] = 3;
            json_req[JKEY_BODY]["map_info_path"] = "/userdata/map/housemap_info3.txt";
            json_req[JKEY_BODY]["map_data_path"] = "/userdata/map/housemap3.txt";
            Json2String(json_req, str_req);
            netserver_client_->SendDpMessage(DP_SCHEDULE_BROADCAST, 0, str_req.c_str(), str_req.size(), &str_res);
            break;
        }
        default:
            break;
        }
    }
    
    
    
}
void CRobotSchedule::timerProcess()
{
    printf("timerProcess start\n");
	while(m_schedule)
	{
		m_timer.DetectTimers();
		usleep(10*1000);
        if (getCRobotdata() == false)
        {
            break;
        }
        
	}
}
void CRobotSchedule::sendState()
{
    chml::DpClient::Ptr netserver_client_ = NetserverInstruct::getNetInstruct()->GetDpnetserverClientPtr();
    auto device_state = CDataModel::getDataModel()->getMState();
    auto clean_state = CDataModel::getDataModel()->getCleanState();
    DLOG_INFO(MOD_BUS, "senddevicestate");
    std::string str_req, str_res;
    Json::Value json_req;
    json_req[JKEY_TYPE] = "robot_device_state";
    json_req[JKEY_BODY][JKEY_DEVICE_STATE] = device_state;
    json_req[JKEY_BODY][JKEY_CLEAN_STATE] = clean_state;
    Json2String(json_req, str_req);
    netserver_client_->SendDpMessage(DP_SCHEDULE_BROADCAST, 0, str_req.c_str(), str_req.size(), &str_res);
    
}
bool CRobotSchedule::init()
{
    printf("CRobotSchedule init start\n");
   
    m_timer.register_timer([](uint64_t){CRobotSchedule::getRobotSchedule()->listenState();}, 1000, 1000, REPEAT_FOREVER);
    //m_timer.register_timer([](uint64_t){CRobotSchedule::getRobotSchedule()->sendmapdata();}, 5000, 1000, REPEAT_FOREVER);
    m_cleanTh = new std::thread(std::bind(&CRobotSchedule::cleanSchedule, this));
    //m_timerTh = new std::thread(std::bind(&CRobotSchedule::timerProcess,this));
    printf("cthread thread ok\n");
    sleep(1);

    return true;
}

void CRobotjobEvent::setJobCommand(EJobCommand cmd)
{

    if (job_cmd == cmd)
    {
        return;
    }
    job_cmd = cmd;
    Ejobcmd.push(cmd);
    printf("push cmd=%d\n",Ejobcmd.back());
    if(Ejobcmd.size() > 2)
    {
        Ejobcmd.pop();
    }
    


}
EJobCommand CRobotjobEvent::getJobCommand()
{
    int m_cmd = job_cmd;
    if(Ejobcmd.empty() == true)
    {
        /* code */
        return (EJobCommand)m_cmd;
    }
    else
    {
        auto m_cmd = Ejobcmd.back();
        //job_cmd = cmd;
        //printf("cmd=%d\n",cmd);
        return (EJobCommand)m_cmd;
    }
    
  
}
CRobotSchedule *CRobotSchedule::getRobotSchedule()
{
    if (m_RobotSchedule == nullptr)
    {
        m_scheduleMutex.lock();
        if (m_RobotSchedule == nullptr)
        {
            m_RobotSchedule = new CRobotSchedule();
        }
    
        m_scheduleMutex.unlock();
    }

    return m_RobotSchedule;
}
CRobotSchedule *CRobotSchedule::delRobotSchedule()
{
    if (m_RobotSchedule)
    {
       
        delete(m_RobotSchedule);
        m_RobotSchedule = nullptr;
    }
    
}
