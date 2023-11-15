/**
 * @Description: 各模块基类
 * @Author:      zhz
 * @Version:     1
 */

#ifndef SRC_LIB_NET_SERVER_BASE_MODULE_H_
#define SRC_LIB_NET_SERVER_BASE_MODULE_H_

#include "core_include.h"

namespace net {

#define DP_MSG_HDR(class_name, map_name, hdr_name) \
typedef int(class_name::*hdr_name) ( \
  Json::Value& req_json, \
  Json::Value& resp_json); \
typedef struct { \
  const char *type; \
  hdr_name handler; \
} map_name;

class NetServer;
class BaseModule {
 public:
  explicit BaseModule(NetServer* net_server_ptr)
     : netptr_(net_server_ptr) {
  }
  virtual ~BaseModule() {}
  virtual int Start() = 0;
  virtual int Stop() = 0;
  virtual int OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) { return -1; }
 
protected:
  NetServer* netptr_;
};

} // namespace net
#endif