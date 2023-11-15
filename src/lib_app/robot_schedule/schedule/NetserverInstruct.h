#ifndef R0B0T_NETSERVER_INSTRUCT_H
#define R0B0T_NETSERVER_INSTRUCT_H
#include "eventservice/base/sigslot.h"
#include "DeviceStates.h"
#include "RobotSchedule.h"
#include "SensorData.h"
#include "json/json.h"
#include "eventservice/dp/dpclient.h"
#include "eventservice/net/eventservice.h"
#include <fstream>
#include <sstream>
#include <ext/stdio_filebuf.h>
#include <ctime>
#include "DayTime.h"
class NetserverInstruct : public sigslot::has_slots<>
{
private:
    /* data */
    NetserverInstruct();
    struct robot_pose
    {
        double pose_x;
        double pose_y;
        int pose_yam;
    };
    
    chml::DpClient::Ptr dp_netInstruct_client_;
    chml::EventService::Ptr MynetInstruct_Service_;
    static NetserverInstruct *m_netinstruct;
    Timer<WheelTimer> net_timer;
    std::thread *net_timerTh;
    //vector<robot_pose> pose_vector;
    //std::list<robot_pose> pose_list;
    std::vector<robot_pose> pose_vector;
    time_t work_time;
    //std::queue<robot_pose> pose_queue;
public:
    ~NetserverInstruct();
    Timer<WheelTimer>* timer()
    {
        return &net_timer;
    }
    static std::mutex m_netMutex;
    static NetserverInstruct *getNetInstruct();
    void OnDpNet_Message(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg);
    bool init();
    bool work_handle(std::string &netype, Json::Value& jreq);
    void workstate_handle(std::string &state_type, Json::Value& jres, Json::Value& jreq);
    void time_reckon();
    void send_robot_pose();
    //void send_clean_trail();
    void send_clean_log();
    void pose_down_net(int &pose_x,int &pose_y,int &pose_yam);
    void send_clean_area();
    void netserverIntergrationHandle(EJobCommand job_cmd,E_MASTER_STATE m_state,std::string play_voice);
    chml::DpClient::Ptr GetDpnetserverClientPtr();

};

#endif