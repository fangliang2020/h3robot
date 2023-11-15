#ifndef DBDATA_BASE_H
#define DBDATA_BASE_H
#include "NetserverInstruct.h"
#include "robot_schedule/base/base_module.h"
#include "SensorData.h"
#include "robot_schedule/clean/control_interface.hpp"
#include <unistd.h>
class DbdataBase : public sigslot::has_slots<>
{
    private:
    DbdataBase();
    chml::DpClient::Ptr dp_dbdata_client_;
    chml::EventService::Ptr Mydbdata_Service_;
    static DbdataBase *m_dbdata_base;
    int clean_mode;
    int disturb_start_time;
    int disturb_end_time;
    int reservation_value;
    int reservation_real;
    int donot_disturb_value;
    struct Clean_value
    {
        int day;
        int enable;
        int start_time;
    };
    // struct Room
    // {
    //     int vertexs_value[4][2];
    //     std::string bedroom_name;
    //     int room_id;
    // };
   
    std::vector<Clean_value> time_clean;
    Timer<WheelTimer> db_timer;
    std::thread *db_timerTh;
    // std::vector<Room> room_vertexs;
    // std::vector<Room> virtual_wall;
    // std::vector<Room> forbidden_zone;
    // std::vector<Room> virtual_threshold;
    public:
    ~DbdataBase();
    static std::mutex db_instMutex;
    static DbdataBase *getDbInstruct();
    void OnDpDb_Message(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg);
    void Dpwork_handle(std::string &dptype,Json::Value& dpjreq);
    bool init();
    int get_disturb_start_time();
    int get_disturb_end_time();
    void Clean_type(Json::Value &jreq, Json::Value &jret);
    void Clean_strength(Json::Value &jreq, Json::Value &jret);
    void Time_clean(Json::Value &jreq, Json::Value &jret);
    void Donot_disturb(Json::Value &jreq, Json::Value &jret);
    void Self_clean(Json::Value &jreq, Json::Value &jret);
    void Zone_cfg(Json::Value &jreq, Json::Value &jret);
    void Delete_map(Json::Value &jreq, Json::Value &jret);
    void save_db_map(int &map_id);
    int get_clean_type();
    void work_clean_type();
    void work_clean_strength();
    int get_donot_disturb_cfg();
    int get_timing_clean_cfg();
    void get_zone_cfg();
    int get_map_number();
    void saveAllmaponDb();
    void dbCfgtime();
    void dbCfgwork();

    chml::DpClient::Ptr GetDpdbBaseClientPtr();
};


#endif