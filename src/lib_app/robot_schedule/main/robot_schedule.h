#ifndef MAIN_SHEDULE_H
#define MAIN_SHEDULE_H
#include "robot_schedule/schedule/RobotSchedule.h"
#include "eventservice/net/eventservice.h"
#include "robot_schedule/schedule/SensorData.h"
#include "robot_schedule/schedule/NetserverInstruct.h"
#include "robot_schedule/schedule/DbdataBase.h"
int init_schedule(chml::EventService::Ptr event_service);



#endif