#include "time_util.h"
#include "eventservice/log/log_client.h"
#ifdef POSIX
#include <sys/time.h>
#if defined(OSX) || defined(IOS)
#include <mach/clock.h>
#include <mach/mach_time.h>
#endif
#endif

#ifdef WIN32
// #define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <time.h>
#endif

namespace chml {

#ifdef WIN32
int gettimeofday(struct timeval *tp, void *tzp) {
  time_t clock;
  struct tm tm;
  SYSTEMTIME wtm;

  GetLocalTime(&wtm);
  tm.tm_year    = wtm.wYear - 1900;
  tm.tm_mon    = wtm.wMonth - 1;
  tm.tm_mday    = wtm.wDay;
  tm.tm_hour    = wtm.wHour;
  tm.tm_min    = wtm.wMinute;
  tm.tm_sec    = wtm.wSecond;
  tm. tm_isdst    = -1;
  clock = mktime(&tm);
  tp->tv_sec = clock;
  tp->tv_usec = wtm.wMilliseconds * 1000;

  return (0);
}
#endif

uint64 XTimeUtil::ClockNanoSecond() {
  int64 ticks = 0;
#if defined(OSX) || defined(IOS)
  static mach_timebase_info_data_t timebase;
  if (timebase.denom == 0) {
    // Get the timebase if this is the first time we run.
    // Recommended by Apple's QA1398.
    ML_VERIFY(KERN_SUCCESS == mach_timebase_info(&timebase));
  }
  // Use timebase to convert absolute time tick units into nanoseconds.
  ticks = mach_absolute_time() * timebase.numer / timebase.denom;
#elif defined(POSIX)
  struct timespec ts;
  // Do we need to handle the case when CLOCK_MONOTONIC
  // is not supported?
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ticks = 1000000000 * static_cast<int64>(ts.tv_sec) + static_cast<int64>(ts.tv_nsec);
#elif defined(WIN32)
  static volatile LONG last_timegettime = 0;
  static volatile int64 num_wrap_timegettime = 0;
  volatile LONG *last_timegettime_ptr = &last_timegettime;
  DWORD now = timeGetTime();
  // Atomically update the last gotten time
  DWORD old = InterlockedExchange(last_timegettime_ptr, now);
  if (now < old) {
    // If now is earlier than old, there may have been a race between
    // threads.
    // 0x0fffffff ~3.1 days, the code will not take that long to execute
    // so it must have been a wrap around.
    if (old > 0xf0000000 && now < 0x0fffffff) {
      num_wrap_timegettime++;
    }
  }
  ticks = now + (num_wrap_timegettime << 32);
  // Calculate with nanosecond precision.  Otherwise, we're just
  // wasting a multiply and divide when doing Time() on Windows.
  ticks = ticks * 1000000;
#endif
  return ticks;
}

// second
uint64 XTimeUtil::TimeSecond() {
  return time(NULL);
}

//
uint64 XTimeUtil::TimeUTCMake(const TimeS *tl) {
  struct tm tp = {0};

  tp.tm_sec = tl->sec;
  tp.tm_min = tl->min;
  tp.tm_hour = tl->hour;
  tp.tm_mday = tl->day;
  tp.tm_mon = tl->month - 1;
  tp.tm_year = tl->year - 1900;
  tp.tm_wday = tl->wday;
  tp.tm_isdst = -1;
  return (uint64) mktime(&tp);
}

void XTimeUtil::TimeUTCMake(TimeS *tl, uint64 sec) {
  if (0 == sec) {
    sec = TimeSecond();
  }
  struct tm *tm = NULL;
#ifdef WIN32
  // On Windows, gmtime is thread safe.
  struct tm t_tm;
  gmtime_s(&t_tm, (time_t*)&sec);
  tm = &t_tm;
#else
  tm = gmtime((time_t*)&sec);  // NOLINT
#endif

  if (tm && tl) {
    tl->usec = 0;
    tl->sec = (uint32) tm->tm_sec;
    tl->min = (uint32) tm->tm_min;
    tl->hour = (uint32) tm->tm_hour;
    tl->day = (uint32) tm->tm_mday;
    tl->month = (uint32) tm->tm_mon + 1U;
    tl->year = (uint32) tm->tm_year + 1900U;
    tl->wday = (uint32) tm->tm_wday;
  }
}

int32 XTimeUtil::TimeLocalZone() {
#ifdef WIN32
  time_t time_utc;
  time(&time_utc);

  int time_zone = 0;
  //struct tm tm_local, tm_gmt;
  //localtime(&tm_local, &time_utc);
  //gmtime(&tm_gmt, &time_utc);
  struct tm *ptm_gmtime = gmtime(&time_utc);
  struct tm *ptm_local = localtime(&time_utc);
  if (ptm_local && ptm_gmtime) {
    time_zone = ptm_local->tm_hour - ptm_gmtime->tm_hour;
  }
  if (time_zone < -12) {
    time_zone = time_zone + 24;
  } else if (time_zone > 12) {
    time_zone = time_zone - 24;
  }
  return time_zone * 60;
#else
#ifdef DEVICE_SDK
  return ML_DeviceSDK_Get_TimeZone();
#else
  return 0;
#endif
#endif
}

// TimeLocal -> seconds
uint64 XTimeUtil::TimeLocalMake(const TimeS *tl) {
  uint64 sec = TimeUTCMake(tl);
  int32 time_zone = TimeLocalZone();
  sec = sec - time_zone * 60;
  return sec;
}

// seconds -> TimeLocal
void XTimeUtil::TimeLocalMake(TimeS *tl, uint64 sec) {
  if (0 == sec) {
    sec = TimeSecond();
  }
#ifdef WIN32
  // windows的时间戳和LITEOS相同，转换需要按照0时区调整。
  int32 time_zone = 0;
  time_zone = TimeLocalZone();
  int32 time_zone_diff = time_zone * 3600;
  sec = sec - time_zone_diff;
#endif
  // 获取本地时间,即经过时区转换的时间
  time_t secs = sec;
  struct tm *tp = localtime(&secs);
  if (tp && tl) {
    tl->usec = TimeUSecond()%1000000;
    tl->sec = (uint32) tp->tm_sec;
    tl->min = (uint32) tp->tm_min;
    tl->hour = (uint32) tp->tm_hour;
    tl->day = (uint32) tp->tm_mday;
    tl->month = (uint32) tp->tm_mon + 1U;
    tl->year = (uint32) tp->tm_year + 1900U;
    tl->wday = (uint32) tp->tm_wday;
  }
}

// TimeLocal -> useconds
uint64 XTimeUtil::TimeUSLocalMake(const TimeS *tl) {
  uint64 sec = TimeUTCMake(tl);
  int32 time_zone = TimeLocalZone();
  sec = sec - time_zone * 60;
  return sec * 1000000L + tl->usec;
}

// useconds -> TimeLocal
void XTimeUtil::TimeUSLocalMake(TimeS *tl, uint64 usec) {
  if (0 == usec) {
    usec = TimeUSecond();
  }
#ifdef WIN32
  // windows的时间戳和LITEOS相同，转换需要按照0时区调整。
  int32 time_zone = 0;
  time_zone = TimeLocalZone();
  int32 time_zone_diff = time_zone * 3600;
  usec = usec - time_zone_diff;
#endif
  // 获取本地时间,即经过时区转换的时间
  time_t secs = usec / 1000000ULL;
  struct tm *tp = localtime(&secs);
  if (tp && tl) {
    tl->usec = usec % 1000000ULL;
    tl->sec = (uint32) tp->tm_sec;
    tl->min = (uint32) tp->tm_min;
    tl->hour = (uint32) tp->tm_hour;
    tl->day = (uint32) tp->tm_mday;
    tl->month = (uint32) tp->tm_mon + 1U;
    tl->year = (uint32) tp->tm_year + 1900U;
    tl->wday = (uint32) tp->tm_wday;
  }
}

// seconds -> "2020-01-06 17:44:22"
std::string XTimeUtil::TimeLocalToString(uint64 sec) {
  TimeS tl;
  TimeLocalMake(&tl, sec);

  char tm_s[24] = {0};
  snprintf(tm_s, 23,
           "%04d-%02d-%02d %02d:%02d:%02d",
           tl.year, tl.month, tl.day, tl.hour, tl.min, tl.sec);
  return tm_s;
}

// "2020-01-06 17:44:22" -> seconds
uint64 XTimeUtil::TimeLocalFromString(const char* str) {
  tm convert_tm;
  memset((void *)&convert_tm, 0, sizeof(tm));
  int res = sscanf(str, "%4d%*[^0-9]%2d%*[^0-9]%2d%*[^0-9]%2d%*[^0-9]%2d%*[^0-9]%2d",
    &convert_tm.tm_year,
    &convert_tm.tm_mon,
    &convert_tm.tm_mday,
    &convert_tm.tm_hour,
    &convert_tm.tm_min,
    &convert_tm.tm_sec);

  if (0 >= (int)(convert_tm.tm_mon -= 2)) {    /* 1..12 -> 11,12,1..10 */
    convert_tm.tm_mon += 12;      /* Puts Feb last since it has leap day */
    convert_tm.tm_year -= 1;
  }
  time_t temp_time = (((((time_t)(convert_tm.tm_year / 4 - convert_tm.tm_year / 100 + convert_tm.tm_year / 400
    + 367 * convert_tm.tm_mon / 12 + convert_tm.tm_mday) +
    convert_tm.tm_year * 365 - 719499) * 24 + convert_tm.tm_hour - 8/* now have hours */
    ) * 60 + convert_tm.tm_min /* now have minutes */
    ) * 60 + convert_tm.tm_sec);
  return temp_time;
}

int32 XTimeUtil::TimeToSecond(const char* str) {
  int32 hour = 0, minute = 0, second = 0;
  if (3 == sscanf(str, "%02d:%02d:%02d", &hour, &minute, &second)
      && hour >= 0 && hour <= 24
      && minute >= 0 && minute < 60
      && second >= 0 && second < 60) {
    return second + minute * 60 + hour * 3600;
  }
  return -1;
}

uint64 XTimeUtil::TimeMSecond() {
  uint64 msec = 0;
#ifdef WIN32
  // windows下无法获取精确的usec，每次递增1微秒来区分时间，
  // 但概率存在时间反转
  static uint32 offset = 0;
  offset = (offset + 1) & (31);
  msec = (uint64)chml::XTimeUtil::TimeSecond() * 1000;
  msec += offset;
#else
  struct timeval tv = { 0 };
  gettimeofday(&tv, NULL);
  msec = (uint64)tv.tv_sec * 1000;
  msec += (uint32_t)tv.tv_usec / 1000;
#endif
  return msec;
}

  // usecond
uint64 XTimeUtil::TimeUSecond() {
  uint64 usec = 0;
#ifdef WIN32
  // windows下无法获取精确的usec，每次递增1微秒来区分时间，
  // 但概率存在时间反转
  static uint32 offset = 0;
  offset = (offset + 1) & (31);
  usec = (uint64)chml::XTimeUtil::TimeSecond() * 1000000;
  usec += offset;
#else
  struct timeval tv = { 0 };
  gettimeofday(&tv, NULL);
  usec = tv.tv_sec * 1000000ULL + tv.tv_usec;
#endif
  return usec;
}

std::string XTimeUtil::TimeMSLocalToString(uint64 msec) {
  TimeS tl;
  TimeUSLocalMake(&tl, msec * 1000);
  char tm_s[64] = { 0 };
  snprintf(tm_s, sizeof(tm_s),"%02d:%02d:%02d.%03d", tl.hour, tl.min, tl.sec, tl.usec / 1000);
  return tm_s;
}

char *XTimeUtil::TimeMSLocalToString(char *str, int32_t size, uint64 msec) {
  TimeS tl;
  TimeUSLocalMake(&tl, msec * 1000);
  snprintf(str, size,"%02d:%02d:%02d.%03d", tl.hour, tl.min, tl.sec, tl.usec / 1000);
  return str;
}

XProcTime::XProcTime(const char *msg, int log_tag) {
  this->msg_ = strdup(msg);
  gettimeofday(&s_tv_, NULL);
  memcpy(&p_tv_, &s_tv_, sizeof(struct timeval));
}

void XProcTime::Check(const char *msg) {
  gettimeofday(&e_tv_, NULL);
  uint64_t diff_usec = (e_tv_.tv_sec - p_tv_.tv_sec) * 1000000L + (e_tv_.tv_usec - p_tv_.tv_usec);
  DLOG_DEBUG(log_tag_, "%s, spend time: %lld.%lld msec", msg, diff_usec / 1000L, diff_usec % 1000L);
  memcpy(&p_tv_, &e_tv_, sizeof(struct timeval));
}

XProcTime::~XProcTime() {
  gettimeofday(&e_tv_, NULL);
  uint64_t diff_usec = (e_tv_.tv_sec - s_tv_.tv_sec) * 1000000L + (e_tv_.tv_usec - s_tv_.tv_usec);
  DLOG_DEBUG(log_tag_, "%s, spend time: %lld.%lld msec", msg_, diff_usec / 1000L, diff_usec % 1000L);
  free(msg_);
  msg_ = NULL;
}

}  // namespace chml
