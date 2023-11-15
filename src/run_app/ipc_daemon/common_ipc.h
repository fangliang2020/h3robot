#ifndef __COMMON_IPC_H__
#define __COMMON_IPC_H__

#include "core_include.h"


#define JKEY_ID                 "id"
#define JKEY_FROM               "from"
#define JKEY_FROM_NAME          "from_name"
#define JKEY_TO_NAME            "to_name"
#define JKEY_TYPE               "type"
#define JKEY_CONTENT            "content"

#define JKEY_SSID               "ssid"
#define JKEY_PASSWORD           "password"
#define JKEY_TIMEZONE           "timezone"
#define JKEY_TIME               "time"

#define JKEY_VERSION            "version"
#define JKEY_MAC                "mac"

#define JKEY_BODY               "body"
#define JKEY_GEAR               "gear"
#define JKEY_STATUS             "status"
#define JKEY_STATE              "state"
#define JKEY_ERR                "err_msg"
// #ifdef __cplusplus
// extern "C" {
// #endif

#define ASSERT_Return_Failure(condition, res)                                 \
  if (condition) {                                                            \
    printf("ASSERT (" #condition ") is true, then return " #res); \
    return res;                                                               \
  }

#define ASSERT_Return_Void(condition)                                     \
  if (condition) {                                                        \
    printf("ASSERT (" #condition ") is true, then returned"); \
    return;                                                               \
  }

#define DP_MSG_HDR(class_name, map_name, hdr_name) \
typedef int(class_name::*hdr_name) ( \
  Json::Value& req_json, \
  Json::Value& resp_json); \
typedef struct { \
  const char *type; \
  hdr_name handler; \
} map_name;

int copy(const char *src, const char *dst);

// #ifdef __cplusplus
// }
// #endif

#endif
