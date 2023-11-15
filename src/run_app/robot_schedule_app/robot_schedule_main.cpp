/**
 * @Description: robot_schedule server main
 * @Author:      djw
 * @Version:     1
 */

#include "app/app/app.h"
#include "robot_schedule/main/robot_schedule.h"
#include <iostream>
#include <string>
#include <thread>
// #include <pthread.h>
#include "robot_schedule/schedule/DeviceStates.h"
#include "robot_schedule/socket/navigate.h"
#include "robot_schedule/schedule/DateModel.h"
#include "robot_schedule/clean/config.hpp"
#include "robot_schedule/clean/control_interface.hpp"
//#include "signal.h"
#include "robot_schedule/schedule/SensorData.h"
#include "robot_schedule/clean/debug.hpp"

extern void backtrace_init();
int main(int argc,char *argv[]) {
  printf("peripherals server app compile time: %s %s.\n", __DATE__, __TIME__);
  backtrace_init();
  bool bret = Log_ClientInit();
  //设置打印等级
  app::App::DebugPrint(argc, argv);  
  //设置当前主线程的名字
   chml::EventService::Ptr evt_srv = 
     chml::EventService::CreateCurrentEventService("robot_schedule_app");
    
    if (evt_srv == nullptr)
    {
        std::cout << "CreateCurrentEventService error" << std::endl;
    }
    
    debug_zlog_init();
    LOG_INFO("navigate start\n");
    thread navigate(navigateloop);
    sleep(3);
    
    LOG_INFO("navigate end\n");
    register_setVWtoRobot(setVWtoRobot);
    register_send_cmd(send_cmd);
    register_get_route(get_route);
    register_followwall_maps_upload(followwall_maps_upload);
    CDataModel::getDataModel()->setMState(ESTATE_IDLE);
    CDataModel::getDataModel()->setCleanState(CLEAN_STATE_NULL);
    auto init_value = init_schedule(evt_srv);

  return init_value;


}