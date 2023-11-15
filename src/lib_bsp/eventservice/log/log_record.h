#ifndef __LOG_RECORD_C_H__
#define __LOG_RECORD_C_H__

#include "eventservice/util/time_util.h"
#include "sys_log.h"

//#ifdef _OPEN_LOG_RECORD_
#if 0
// 普通打印
#define DPRINTF( __format__, ...) printf("%s:%d ==> " __format__ "\n", __FILENAME__, __LINE__, ##__VA_ARGS__)
// 初始化记录时间
#define BEGIN_TIME_RECORD(__start_time__) uint64 __start_time__ = chml::XTimeUtil::ClockMSecond()
#define RESET_TIME_RECORD(__start_time__) __start_time__ = chml::XTimeUtil::ClockMSecond()
// 打印耗时信息
#ifdef WIN32
#define PRINT_TIME_SPAN(__start_time__, __format__, ...) printf("%s:%d timespan:%llu===> " __format__ "\n", __FILENAME__, __LINE__, (chml::XTimeUtil::ClockMSecond() - __start_time__), ##__VA_ARGS__)
#define PRINT_TIME_SPAN_IF(__start_time__, __span_factor__, __format__, ...) do {\
  uint64 __time_span__ = chml::XTimeUtil::ClockMSecond() - __start_time__;\
  if (__time_span__ >= __span_factor__)\
    printf("%s:%d timespan:%llu===> " __format__ "\n", __FILENAME__, __LINE__, __time_span__, ##__VA_ARGS__);\
} while (0)
#else
#define PRINT_TIME_SPAN(__start_time__, __format__, ...) printf("\033[1;36;40m%s:%d timespan:%llu===> " __format__ "\033[0m\n", __FILENAME__, __LINE__, (chml::XTimeUtil::ClockMSecond() - __start_time__), ##__VA_ARGS__)
#define PRINT_TIME_SPAN_IF(__start_time__, __span_factor__, __format__, ...) do {\
  uint64 __time_span__ = chml::XTimeUtil::ClockMSecond() - __start_time__;\
  if (__time_span__ >= __span_factor__)\
    printf("\033[1;36;40m%s:%d timespan:%llu===> " __format__ "\033[0m\n", __FILENAME__, __LINE__, __time_span__, ##__VA_ARGS__);\
} while (0)
#endif
#define POINT_TIME_SPAN_IF(__start_time__, __span_factor__, __logid__, __format__, ...) do {\
  uint64 __time_span__ = chml::XTimeUtil::ClockMSecond() - __start_time__;\
  if (__time_span__ >= __span_factor__)\
    LOG_TRACE_DEV(__logid__, "%s:%d timespan:%llu===>" __format__, __FILENAME__, __LINE__, __time_span__, ##__VA_ARGS__);\
} while (0)
#else
//
#define DPRINTF( __format__, ...) do {} while (0)
// 
#define BEGIN_TIME_RECORD(__start_time__) do {} while (0)
#define RESET_TIME_RECORD(__start_time__) do {} while (0)
//
#define PRINT_TIME_SPAN(__start_time__, __format__, ...) do {} while (0)
//
#define PRINT_TIME_SPAN_IF(__start_time__, __span_factor__, __format__, ...) do {} while (0)
#define POINT_TIME_SPAN_IF(__start_time__, __span_factor__, __logid__, __format__, ...) do {} while (0)
#endif

#endif
