/**
 * @Description: 各模块基类
 * @Author:      zhz
 * @Version:     1
 */

#ifndef SRC_LIB_SCHEDULE_SERVER_BASE_MODULE_H_
#define SRC_LIB_SCHEDULE_SERVER_BASE_MODULE_H_

#include "core_include.h"

//namespace robotshcedule {

#define DP_MSG_HDR(class_name, map_name, hdr_name) \
typedef void(class_name::*hdr_name) ( \
  Json::Value& req_json, \
  Json::Value& resp_json); \
typedef struct { \
  const char *type; \
  hdr_name handler; \
} map_name;

// class ScheduleServer;
// class BaseModule {
//  public:
//   explicit BaseModule(ScheduleServer* net_schedule_ptr)
//      : schptr_(net_schedule_ptr) {
//   }
//   virtual ~BaseModule() {}
//   virtual int Start() = 0;
//   virtual int Stop() = 0;
//   virtual int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) { return -1; }
 
// protected:
//   ScheduleServer* schptr_;
// };


#endif