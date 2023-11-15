/**
 * @Description: net server
 * @Author:      fl
 * @Version:     1
 */

#include "robot_schedule.h"
#include "signal.h"
bool SIG_VALUE = true;
void sigint_handler(int sig)
{
    if (sig == SIGINT)
    {
        /* code */
        SIG_VALUE = false;
        CRobotSchedule::getRobotSchedule()->setCRobotdata(false);
        SensorData::getsensorData()->set_sensor_data(false);

        // interrupt_work();
    }
}
int init_schedule(chml::EventService::Ptr event_service)
{
    printf("init_schedule start\n");
    // if (!sChassisControl::instance()->init()) // 底盘外设初始化，速度设置为0，托布抬起
    // {
    //     printf("sChassisControl init error\n");
    //     return -1;
    // }
    //DbdataBase::getDbInstruct()->get_zone_cfg();
    if (!NetserverInstruct::getNetInstruct()->init())
    {
        printf("NetserverInstruct init error\n");
        return -1;
    }
    if (!DbdataBase::getDbInstruct()->init())
    {
        printf("DbdataBase init error\n");
        return -1;
    }
    
    if (!CRobotSchedule::getRobotSchedule()->init()) //
    {
        printf("sCRobotSchedule init error\n");
        return -1;
    }
    
    // 创建按键触发线程
    // m_signTh = new std::thread(std::bind(&CRobotSchedule::listenState, this));
    // 传感器数据处理线程
    if (!SensorData::getsensorData()->init())
    {
        std::cout << "SensorData init error" << std::endl;
        return -1;
    }
    // if (query_now_mode() == 1)
    // {
    //     DbdataBase::getDbInstruct()->save_db_map();
    // }
    

    signal(SIGINT, sigint_handler);
    while (true)
    {
        /* code */
        if (SIG_VALUE == false)
        {
            std::cout << "ctrl+c exit" << std::endl;
            //setVWtoRobot(0, 0);
            //sleep(1);
            interrupt_work();
            sleep(2);
            SensorData::getsensorData()->delsensorData();
            //SensorData::getsensorData()->~SensorData();
           // sleep(2);
            std::cout << "qqqqqqqq" << std::endl;
            //SensorData::getsensorData()->delsensorData();
            
            
            CRobotjobEvent::getjobEvent()->deljobEvent();
            CRobotSchedule::getRobotSchedule()->~CRobotSchedule();
           
            break;
        }
        usleep(50*1000);
    }
    return 0;
}