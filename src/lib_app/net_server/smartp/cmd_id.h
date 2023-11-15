/**
 * @Description: app 通信
 * @Author:      zhz
 * @Version:     1
 */

#ifndef SRC_LIB_SMARTP_PROTOCOL_H_
#define SRC_LIB_SMARTP_PROTOCOL_H_

// cmd id
#define CMD_ID_QUERY_MAP                     1312
#define CMD_ID_READ_MAP                      1313
#define CMD_ID_UPLOAD_MAP                    1219
#define CMD_ID_CHANGE_MAP_NAME               1311
#define CMD_ID_START_CLEAN                   1020
#define CMD_ID_UPDATE_DEV_STATE              889
#define CMD_ID_START_ZONING_CLEAN            1030
#define CMD_ID_PAUSE_CLEAN                   1060
#define CMD_ID_CONTINUE_CLEAN                1070
#define CMD_ID_RECHARGE                      1050
#define CMD_ID_GET_CLEAR_MODE                1309 // 清扫模式
#define CMD_ID_SET_CLEAR_MODE                1310
#define CMD_ID_GET_CLEAN_MODE                1209 // 清洁模式
#define CMD_ID_SET_CLEAN_MODE                1300
#define CMD_ID_GET_AUDIO_CFG                 1501
#define CMD_ID_SET_AUDIO_CFG                 1502
#define CMD_ID_GET_DONOT_DISTURB_CFG         1224
#define CMD_ID_SET_DONOT_DISTURB_CFG         1225
#define CMD_ID_GET_TIMING_CLEAN_CFG          1222
#define CMD_ID_SET_TIMING_CLEAN_CFG          1223
#define CMD_ID_GET_SELF_CLEAN_CFG            1200
#define CMD_ID_SET_SELF_CLEAN_CFG            1201
#define CMD_ID_GET_ZONE_CFG                  1227
#define CMD_ID_SET_ZONE_CFG                  1226
#define CMD_ID_UPDATE_BATTERY_LEVEL          60
#define CMD_ID_UPDATE_AREA_AND_TIME          1422
#define CMD_ID_UPDATE_ALARM                  600
#define CMD_ID_GET_LOG                       700
#define CMD_ID_UPDATE_CLEAN_TRAJECTORY       69
#define CMD_ID_SELECT_MAP                    1051
#define CMD_ID_FAST_BUILD_MAP                1052
#define CMD_ID_DELETE_MAP                    1053
#define CMD_ID_RESTORE_DEFAULT               1054
#define CMD_ID_UPDATE_CLEAN_LOG              1055
#define CMD_ID_UPDATE_DEVICE_POSE            1056
#define CMD_ID_UPDATE_TOF_BUMP               1057
#define CMD_ID_FORCE_TIME                    1058
#define CMD_ID_GET_ALL_STATE                 1059
#define CMD_ID_UPDATE_SELF_CLEAN_PERCENTAGE  61
#define CMD_ID_RESET_FACTORY                 1061
#define CMD_ID_OTA_UPDATE                    1062

#endif // SRC_LIB_SMARTP_PROTOCOL_H_