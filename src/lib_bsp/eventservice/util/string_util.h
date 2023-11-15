#ifndef EVENTSERVICES_UTIL_STRING_UTIL_H
#define EVENTSERVICES_UTIL_STRING_UTIL_H

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef WIN32
#include <malloc.h>
#include <wchar.h>
#ifndef __MINGW32__
#define alloca _alloca
#endif
#endif  // WIN32

#ifdef POSIX
#ifdef BSD
#include <stdlib.h>
#else  // BSD
#include <alloca.h>
#endif  // !BSD
#endif  // POSIX

#include <cstring>
#include <string>
#include <vector>

#include "eventservice/base/basictypes.h"

///////////////////////////////////////////////////////////////////////////////
// Generic string/memory utilities
///////////////////////////////////////////////////////////////////////////////

#define STACK_ARRAY(TYPE, LEN) \
  static_cast<TYPE *>(::alloca((LEN) * sizeof(TYPE)))

namespace chml {

class XStrUtil {
 public:
  // 去除字符串头(或尾)中在字符集中指定的字符
  static std::string &chop_head(std::string &str_src, const char *pcszCharSet = " \t\r\n");
  static std::string &chop_tail(std::string &str_src, const char *pcszCharSet = " \t\r\n");
  static std::string &chop(std::string &str_src, const char *pcszCharSet = " \t\r\n");

  // 字符串转大写(或小写)
  static void to_upper(char *str_src);
  static void to_lower(char *str_src);
  static void to_upper(std::string &str_src);
  static void to_lower(std::string &str_src);

  // 替换
  static void replace(char *str, char oldch, char newch);
  static void replace(char *str, const char *oldCharSet, char newch);

  // 区分大小写比较
  static int32 compare(const char *str1, const char *str2, int32 length = -1);
  static int32 compare(const std::string &str1, const std::string &str2, int32 length = -1);

  // 不区分大小写比较
  static int32 compare_nocase(const char *str1, const char *str2, int32 length = -1);
  static int32 compare_nocase(const std::string &str1, const std::string &str2, int32 length = -1);

  // 根据字符集中指定的分隔字符分解源字符串,并放置到std::vector中
  // nMaxCount指定期望得到的行数,解析到maxCount将终止并返回,不会继续解析;设为-1表示解析所有
  static uint32 split(const std::string &str_src,
                        std::vector<std::string> &vItems,
                        const char *pcszCharSet = " \r\n\t",
                        int32 nMaxCount = -1);

  // 字符串转整数
  static bool to_int(const std::string &str_src, int32 &nval, int32 radix = 10);
  static int32 to_int_def(const std::string &str_src, int32 def = -1, int32 radix = 10);
  static int32 try_to_int_def(const std::string &str_src, int32 def = -1, int32 radix = 10);
  static bool to_uint(const std::string &str_src, uint32 &uValue, int32 radix = 10);
  static uint32 to_uint_def(const std::string &str_src, uint32 def = 0, int32 radix = 10);
  static uint32 try_to_uint_def(const std::string &str_src, uint32 def = 0, int32 radix = 10);

  // 字符串转浮点型数
  static bool to_float(const std::string &str_src, double &value);
  static double to_float_def(const std::string &str_src, double def = 0.0);
  static double try_to_float_def(const std::string &str_src, double def = 0.0);

  // 数值转字符串
  static std::string to_str(int32 nVal, const char *fmt = NULL /*"%d"*/);
  static std::string to_str(uint32 uVal, const char *fmt = NULL /*"%u"*/);
  static std::string to_str(int64 nlVal, const char *fmt = NULL /*"%lld"*/);
  static std::string to_str(uint64 ulVal, const char *fmt = NULL /*"%llu"*/);
#if 0
  static std::string to_str(time_t ulVal, const char* fmt = NULL /*"%llu"*/);
#endif
  static std::string to_str(double fVal, const char *fmt = NULL /*"%f"*/);

  // std::string hash
  static uint32 hash_code(const char *str);
  static uint32 murmur_hash(const void *key, uint32 len);

  // calc string checksum
  static uint32 check_sum(const char *value, uint32 len);

  // dump data
  static void dump(std::string &result, const void *pdata, uint32 length);

  //
  static std::string random_string(std::size_t size);

  //
  static char *safe_strtok(char *str, const char *delim, char **saveptr);
  static void safe_memcpy(void *p_dst, unsigned int dst_size, const void *p_src, unsigned int src_size);

  static char hex_encode(unsigned char val);

  static bool hex_decode(char ch, unsigned char* val);

  // url_encode is an encode operation with a predefined set of illegal characters and escape character (for use in URLs, obviously).
  static uint32 url_encode(char* buffer, uint32 buflen, const char* source, uint32 srclen);
  // Note: in-place decoding (buffer == source) is allowed.
  static uint32 url_decode(char* buffer, uint32 buflen, const char* source, uint32 srclen);

  // 十六进制字符串和二进制数据转换
  static std::string bin_2_hex(const std::string& src, bool upper = false);
  static std::string hex_2_bin(const std::string& src);
};

}  // namespace chml

#endif  // EVENTSERVICES_UTIL_STRING_UTIL_H
