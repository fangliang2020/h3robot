/**
 * @Description: 公共宏和结构体
 * @Author:      zhz
 * @Version:     1
 */

#ifndef SRC_LIB_NET_SERVER_BASE_COMMON_H_
#define SRC_LIB_NET_SERVER_BASE_COMMON_H_

#define DP_DB_REQUEST                 "DP_DB_REQUEST"
#define DP_SCHEDULE_REQUEST           "DP_SCHEDULE_REQUEST"
#define DP_SCHEDULE_BROADCAST         "DP_SCHEDULE_BROADCAST"                                      
#define DP_PERIPHERALS_REQUEST        "DP_PERIPHERALS_REQUEST"
#define DP_PERIPHERALS_BROADCAST      "DP_PERIPHERALS_BROADCAST"
#define DP_DAEMON_REQUEST             "DP_DAEMON_REQUEST"

#define DP_TYPE_SENSOR_DATA           "sensor_data"
#define DP_TYPE_LASER_SENSOR_SCAN     "laser_sensor_scan"
#define DP_TYPE_ASTAR_PLAN            "astar_plan"
#define DP_TYPE_COVER_PLAN            "cover_plan"
#define DP_TYPE_ROBOT_DEVICE_STATE    "robot_device_state"
#define DP_TYPE_TRIGGER_DATA          "trigger_data"
#define DP_TYPE_ROBOT_DEVICE_POSE     "robot_device_pose"
#define DP_TYPE_UPDATE_CLEAN_TRAIL    "update_clean_trail"
#define DP_TYPE_UPDATE_CURRENT_MAP    "update_current_map"
#define DP_TYPE_UPDATE_CLEAN_AREA     "update_clean_area"
#define DP_TYPE_TOF_BUMP_SENSOR       "tof_bump_sensor"
#define DP_TYPE_UPDATE_DEVICE_ERROR   "update_device_error"
#define DP_TYPE_CLEAN_LOG             "clean_log"
#define DP_TYPE_SELF_CLEAN_PERCENTAGE "self_clean_percentage"
#define DP_FAULT_STATE                "fault_state"

#endif