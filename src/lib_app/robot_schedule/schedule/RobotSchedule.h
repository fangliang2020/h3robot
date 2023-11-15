/***************************************/
/****************
 * 机器工作调度
 * @author dingjinwen
 * @date   2023/1/5
 * **********************/
#ifndef R0B0T_SHEDULE_H
#define R0B0T_SHEDULE_H
#include <thread>
#include "Timer.h"
#include <queue>
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include "DeviceStates.h"
#include "singleton.h" 
#include "eventservice/dp/dpclient.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/base/sigslot.h"
#include "robot_schedule/base/common.h"
#include "robot_schedule/base/jkey_common.h"
#include "DbdataBase.h"
//#include "PlayVoice.h"
#include "peripherals_server/audio/voice_play_config.h"
//#include "robot_schedule/base/base_module.h"
//#include "robot_schedule/base/common.h"
using namespace std;
//#include "signal.h"
//#define REPEAT_FOREVER 0
//bool SIG_VALUE = false;
enum E_SLAM_STATE
{
    SLAM_IDLE = 0,
    SLAM_RUN = 1,
    SLAM_PAUSE = 2,
    SLAM_ERROR = 3,
};
class CRobotSchedule : public sigslot::has_slots<>
{
 	public:
	// CRobotSchedule();
    // ~CRobotSchedule();
	~CRobotSchedule();
	Timer<WheelTimer>* timer()
    {
        return &m_timer;
    }
	static std::mutex m_scheduleMutex;
	void cleanSchedule();
	void newCommandHandle();  //获取清扫指令并执行对应的清扫任务
	void cleanWorkHandle();
	void cleanCommand(int cmd);
	void timerProcess();
	void listenState();
	void sendState();
	void sendmapdata();
	//void map_assemble(int &map_id);
	void startWorkSchedule(bool bLoadMap = true);
	void startSlamModule(bool initMap = true);
	void saveSlam_mapdata();
	void getSlam_mapdata();
	void socket_client(char ch);
	void allWorkInterface(EJobCommand e_Cmd,E_MASTER_STATE e_Mstate,CLEAN_STATE e_Cstate,std::string play_voice);
	int getcurrentmap(int mapID);
	int getcleantype();
	int getcleanresult();
	static CRobotSchedule *getRobotSchedule();
    static CRobotSchedule *delRobotSchedule();
	E_SLAM_STATE getSlamStartState()
    {
        return m_slamState;
    }
    bool getCRobotdata();
	void setCRobotdata(bool m_schedule);
	bool init();
	private:
	void startSlam();

	private:
    CRobotSchedule();
	static CRobotSchedule *m_RobotSchedule;
	std::thread *m_cleanTh;
	std::thread *m_timerTh;
	std::thread *m_listenTh;
	std::thread *m_stateTh;
    bool m_schedule = true;
	E_SLAM_STATE m_slamState{SLAM_IDLE};
	std::thread *m_saveMapTh;
	//int m_jobEvent;
	Timer<WheelTimer> m_timer;
	bool SIG_VALUE_SCHEDULE = true;
	std::string mapPatchname;
	unsigned char clean_return_value;
	int clean_type = 0;
	int clean_result = 0;
	
};

class CRobotjobEvent
{
public:
	static CRobotjobEvent* getjobEvent();
	static std::mutex m_jobMutex;
	void deljobEvent();

	//void setjobcommand();
	//void getjobcommand();
	void setJobCommand(EJobCommand cmd);
    EJobCommand getJobCommand();
	std::queue<int> Ejobcmd;
private:
	CRobotjobEvent();
	~CRobotjobEvent();
	static CRobotjobEvent* m_cleanstance;
	int job_cmd;
	

};

#endif
