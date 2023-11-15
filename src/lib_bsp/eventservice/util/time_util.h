#ifndef ALL_BSP_V300_SRC_LIB_EVENTSERVICE_UTIL_TIME_UTIL_H_
#define ALL_BSP_V300_SRC_LIB_EVENTSERVICE_UTIL_TIME_UTIL_H_

#include <string>

#include "eventservice/base/basictypes.h"
#include <boost/shared_ptr.hpp>

namespace chml {

enum enTimingType {
  TT_EMPTY    = 0x00, // 不校时
  TT_NTP      = 0x01, // NTP校时
  TT_ONVIF    = 0x02, // onvif校时
  TT_HTTP     = 0x04, // http校时
  TT_GPS      = 0x08, // GPS校时
  TT_GP28181  = 0x10, // GP28181校时
  TT_SDK      = 0x20, // SDK校时
};

typedef struct _TimeS {
  uint32 usec;   // 微秒，范围: 0~999999
  uint32 sec;    // 秒，范围: 0~59
  uint32 min;    // 分钟，范围: 0~59
  uint32 hour;   // 小时，范围: 0~23
  uint32 day;    // 日，范围: 1~31
  uint32 month;  // 月，范围: 1~12
  uint32 year;   // 年，范围: 1970~...
  uint32 wday;   // 星期，monday,tuesday,...,范围 : 0 ~6
} TimeS;

class XTimeUtil {
 public:
  static uint64 ClockNanoSecond();
  static uint64 ClockMSecond() {
    return ClockNanoSecond() / 1000000;
  }
  static uint32 ClockSecond() {
    return (uint32)(ClockMSecond() / 1000);
  }

  // second
  static uint64 TimeSecond();

  //
  static uint64 TimeUTCMake(const TimeS *tl);
  static void TimeUTCMake(TimeS *tl, uint64 sec = 0);

  // 返回当前windows所在时区（如果是东时区为正，如果是西时区为负）
  static int32 TimeLocalZone();

  // TimeLocal -> seconds
  static uint64 TimeLocalMake(const TimeS *tl);
  // seconds -> TimeLocal
  static void TimeLocalMake(TimeS *tl, uint64 sec = 0);

  // TimeLocal -> useconds
  static uint64 TimeUSLocalMake(const TimeS *tl);

  // useconds -> TimeLocal
  static void TimeUSLocalMake(TimeS *tl, uint64 usec = 0);

  // seconds -> "2020-01-06 17:44:22"
  static std::string TimeLocalToString(uint64 sec = 0);

  // "2020-01-06 17:44:22" -> seconds
  static uint64 TimeLocalFromString(const char* str);

  // "17:44:22" -> seconds
  static int32 TimeToSecond(const char* str);

  // msecond
  static uint64 TimeMSecond();

  // usecond
  static uint64 TimeUSecond();

  static std::string TimeMSLocalToString(uint64 msec = 0);

  static char *TimeMSLocalToString(char *str, int32_t size, uint64 msec = 0);
};

class XProcTime {
 public:
  typedef boost::shared_ptr<XProcTime> Ptr;
  XProcTime(const char *msg, int log_tag);
  ~XProcTime();
  void Check(const char *msg);
 private:
  char *msg_;
  int log_tag_;
  struct timeval s_tv_;
  struct timeval e_tv_;
  struct timeval p_tv_;
};

}  // namespace chml
#endif //ALL_BSP_V300_SRC_LIB_EVENTSERVICE_UTIL_TIME_UTIL_H_
