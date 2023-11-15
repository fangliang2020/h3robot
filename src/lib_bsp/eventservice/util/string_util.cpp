#include "string_util.h"

// #include "eventservice/base/stringutils.h"
// #include "return_define.h"

namespace chml {

std::string &XStrUtil::chop_head(std::string &strSrc, const char *pcszCharSet) {
  if (pcszCharSet == NULL) return strSrc;
  size_t pos = strSrc.find_first_not_of(pcszCharSet);
  return strSrc.erase(0, pos);
}

std::string &XStrUtil::chop_tail(std::string &strSrc, const char *pcszCharSet) {
  if (pcszCharSet == NULL) return strSrc;
  size_t pos = strSrc.find_last_not_of(pcszCharSet);
  if (pos == std::string::npos) {
    strSrc.clear();
    return strSrc;
  }
  return strSrc.erase(++pos);
}

std::string &XStrUtil::chop(std::string &strSrc, const char *pcszCharSet) {
  chop_head(strSrc, pcszCharSet);
  return chop_tail(strSrc, pcszCharSet);
}

void XStrUtil::to_upper(char *pszSrc) {
  if (pszSrc == NULL) return;
  char *pos = (char *)pszSrc;
  char diff = 'A' - 'a';
  while (*pos != '\0') {
    if ('a' <= *pos && *pos <= 'z') {
      *pos += diff;
    }
    pos++;
  }
  return;
}

void XStrUtil::to_lower(char *pszSrc) {
  if (pszSrc == NULL) return;
  char *pos = (char *)pszSrc;
  char diff = 'A' - 'a';
  while (*pos != '\0') {
    if ('A' <= *pos && *pos <= 'Z') {
      *pos -= diff;
    }
    pos++;
  }
  return;
}

void XStrUtil::to_lower(std::string &strSrc) {
  return to_lower((char *)strSrc.c_str());
}

void XStrUtil::to_upper(std::string &strSrc) {
  return to_upper((char *)strSrc.c_str());
}

void XStrUtil::replace(char *str, char oldch, char newch) {
  if (str == NULL || oldch == newch) return;
  char *pos = str;
  while (*pos) {  // (*pos != '\0')
    if (*pos == oldch) *pos = newch;
    pos++;
  }
  return;
}

void XStrUtil::replace(char *str, const char *oldCharSet, char newch) {
  if (str == NULL || oldCharSet == NULL) return;
  char *pos = str;
  const char *p = NULL;
  while (*pos) {  // (*pos != '\0')
    for (p = oldCharSet; *p; p++) {
      if (*pos == *p) {
        *pos = newch;
        break;
      }
    }
    pos++;
  }
  return;
}

int32 XStrUtil::compare(const char *pszSrc1, const char *pszSrc2,
                        int32 length) {
  if (NULL == pszSrc1 || NULL == pszSrc2) {
    printf("param is null");
    return -1;
  }

  int32 ret = 0;
  const char *left = pszSrc1;
  const char *right = pszSrc2;

  while ((length != 0) && (*left != '\0') && (*right != '\0')) {
    if (length > 0) length--;
    ret = *left++ - *right++;
    if (ret != 0) return ret;
  }
  if (length == 0) return 0;
  return (*left - *right);
}

int32 XStrUtil::compare(const std::string &str1, const std::string &str2,
                        int32 length) {
  return compare(str1.c_str(), str2.c_str(), length);
}

int32 XStrUtil::compare_nocase(const char *pszSrc1, const char *pszSrc2,
                               int32 length) {
  if (NULL == pszSrc1 || NULL == pszSrc2) {
    printf("param is null");
    return -1;
  }

  int32 ret = 0;
  const char *left = pszSrc1;
  const char *right = pszSrc2;

  while ((length != 0) && (*left != '\0') && (*right != '\0')) {
    if (length > 0) length--;
    ret = ::tolower(*left++) - ::tolower(*right++);
    if (ret != 0) return ret;
  }
  if (length == 0) return 0;
  return (*left - *right);
}

int32 XStrUtil::compare_nocase(const std::string &str1,
                               const std::string &str2,
                               int32 length /* = -1*/) {
  return compare_nocase(str1.c_str(), str2.c_str(), length);
}

uint32 XStrUtil::split(const std::string &strSrc,
                       std::vector<std::string> &vItems,
                       const char *pcszCharSet /* = " \r\n\t"*/,
                       int32 nMaxCount /* = -1*/) {
  vItems.clear();

  size_t pos_begin = 0;
  size_t pos_end = 0;
  int32 count = 0;
  while (pos_end != std::string::npos) {
    pos_begin = strSrc.find_first_not_of(pcszCharSet, pos_end);
    if (pos_begin == std::string::npos) break;
    pos_end = strSrc.find_first_of(pcszCharSet, pos_begin);
    std::string strTmp(strSrc, pos_begin, pos_end - pos_begin);
    if (!strTmp.empty()) {
      count++;
      vItems.push_back(strTmp);
    }
    if (nMaxCount > 0 && count >= nMaxCount) {
      break;
    }
  }
  return (uint32)vItems.size();
}

bool XStrUtil::to_int(const std::string &strSrc, int32 &nValue,
                      int32 radix /* = 10*/) {
  char *endPtr = 0;
  std::string str = strSrc;

  chop(str);
  if (str.empty()) return false;

  errno = 0;
  nValue = strtol(str.c_str(), &endPtr, radix);
  if (endPtr - str.c_str() != (int32)str.size()) {
    return false;
  }
  if (errno == ERANGE) return false;
  return true;
}

int32 XStrUtil::to_int_def(const std::string &strSrc, int32 def /* = -1*/,
                           int32 radix /* = 10*/) {
  char *endPtr = 0;
  int32 nValue = 0;
  std::string str = strSrc;

  chop(str);
  if (str.empty()) return def;

  errno = 0;
  nValue = strtol(str.c_str(), &endPtr, radix);
  if (endPtr - str.c_str() != (int32)str.size()) {
    return def;
  }
  if (errno == ERANGE) return false;
  return nValue;
}

int32 XStrUtil::try_to_int_def(const std::string &strSrc,
                               int32 def /* = -1*/,
                               int32 radix /* = 10*/) {
  char *endPtr = 0;
  int32 nValue = 0;
  std::string str = strSrc;

  chop(str);
  if (str.empty()) return def;

  errno = 0;
  nValue = strtol(str.c_str(), &endPtr, radix);
  if (endPtr == str.c_str()) {
    return def;
  }
  if (errno == ERANGE) return false;
  return nValue;
}

bool XStrUtil::to_uint(const std::string &strSrc, uint32 &uValue,
                       int32 radix /* = 10*/) {
  char *endPtr = 0;
  std::string str = strSrc;

  chop(str);
  if (str.empty()) return false;

  errno = 0;
  uValue = strtoul(str.c_str(), &endPtr, radix);
  if (endPtr - str.c_str() != (int32)str.size()) {
    return false;
  }
  if (errno == ERANGE) return false;
  return true;
}

uint32 XStrUtil::to_uint_def(const std::string &strSrc, uint32 def /* = 0*/,
                             int32 radix /* = 10*/) {
  char *endPtr = 0;
  uint32 uValue = 0;
  std::string str = strSrc;

  chop(str);
  if (str.empty()) return def;

  errno = 0;
  uValue = strtoul(str.c_str(), &endPtr, radix);
  if (endPtr - str.c_str() != (int32)str.size()) {
    return def;
  }
  if (errno == ERANGE) return false;
  return uValue;
}

uint32 XStrUtil::try_to_uint_def(const std::string &strSrc,
                                 uint32 def /* = 0*/,
                                 int32 radix /* = 10*/) {
  char *endPtr = 0;
  uint32 uValue = 0;
  std::string str = strSrc;

  chop(str);
  if (str.empty()) return def;

  errno = 0;
  uValue = strtoul(str.c_str(), &endPtr, radix);
  if (endPtr == str.c_str()) {
    return def;
  }
  if (errno == ERANGE) return false;
  return uValue;
}

bool XStrUtil::to_float(const std::string &strSrc, double &value) {
  char *endPtr = 0;
  std::string str = strSrc;

  chop(str);
  if (str.empty()) return false;

  errno = 0;
  value = strtod(str.c_str(), &endPtr);
  if (endPtr - str.c_str() != (int32)str.size()) {
    return false;
  }
  if (errno == ERANGE) return false;
  return true;
}

double XStrUtil::to_float_def(const std::string &strSrc,
                              double def /* = 0.0*/) {
  char *endPtr = 0;
  double fValue = 0.0;
  std::string str = strSrc;

  chop(str);
  if (str.empty()) return def;

  errno = 0;
  fValue = strtod(str.c_str(), &endPtr);
  if (endPtr - str.c_str() != (int32)str.size()) {
    return def;
  }
  if (errno == ERANGE) return def;
  return fValue;
}

double XStrUtil::try_to_float_def(const std::string &strSrc,
                                  double def /* = 0.0*/) {
  char *endPtr = 0;
  double fValue = 0;
  std::string str = strSrc;

  chop(str);
  if (str.empty()) return def;

  errno = 0;
  fValue = strtod(str.c_str(), &endPtr);
  if (endPtr == str.c_str()) {
    return def;
  }
  if (errno == ERANGE) return def;
  return fValue;
}

std::string XStrUtil::to_str(int32 nVal, const char *cpszFormat) {
  char buf[128];
  if (cpszFormat) {
    if (strlen(cpszFormat) > 100) {
      printf("Format too long");
      return "";
    }
    sprintf(buf, cpszFormat, nVal);
  } else {
    sprintf(buf, "%d", nVal);
  }
  
  return buf;
}

std::string XStrUtil::to_str(uint32 uVal, const char *cpszFormat) {
  char buf[128];
  if (cpszFormat) {
    if (strlen(cpszFormat) > 100) {
      printf("Format too long");
      return "";
    }
    sprintf(buf, cpszFormat, uVal);
  } else {
    sprintf(buf, "%u", uVal);
  }
  return buf;
}

std::string XStrUtil::to_str(int64 nlVal, const char *cpszFormat) {
  char buf[256];
  if (cpszFormat) {
    if (strlen(cpszFormat) > 240) {
      printf("Format too long");
      return "";
    }
    sprintf(buf, cpszFormat, nlVal);
  } else {
    sprintf(buf, "%lld", nlVal);
  }
  return buf;
}

std::string XStrUtil::to_str(uint64 ulVal, const char *cpszFormat) {
  char buf[256];
  if (cpszFormat) {
    if (strlen(cpszFormat) > 240) {
      printf("Format too long");
      return "";
    }
    sprintf(buf, cpszFormat, ulVal);
  } else {
    sprintf(buf, "%lld", ulVal);
  }

  return buf;
}

std::string XStrUtil::to_str(double fVal, const char *cpszFormat) {
  char buf[256];
  if (cpszFormat) {
    if (strlen(cpszFormat) > 240) {
      printf("Format too long");
      return "";
    }
    sprintf(buf, cpszFormat, fVal);
  } else {
    sprintf(buf, "%f", fVal);
  }
  return buf;
}

#if 0
std::string XStrUtil::to_str(time_t ulVal, const char* cpszFormat) {
  char buf[256];
  if (cpszFormat) {
    if (strlen(cpszFormat) > 240) {
      printf("Format too long");
      return "";
    }
    sprintf(buf, cpszFormat, ulVal);
  } else {
    sprintf(buf, "%lld", ulVal);
  }
  return buf;
}
#endif

uint32 XStrUtil::hash_code(const char *str) {
  if (str == NULL) return 0;

  uint32 h = 0;
  while (*str) {
    h = 31 * h + (*str++);
  }
  return h;
}

uint32 XStrUtil::murmur_hash(const void *key, uint32 len) {
  if (key == NULL) return 0;

  const uint32 m = 0X5BD1E995;
  const uint32 r = 24;
  const uint32 seed = 97;
  uint32 h = seed ^ len;

  // Mix 4 bytes at a time into the hash
  const uint8 *data = (const uint8 *)key;
  while (len >= 4) {
    uint32 k = *(uint32 *)data;
    k *= m;
    k ^= k >> r;
    k *= m;
    h *= m;
    h ^= k;
    data += 4;
    len -= 4;
  }

  // Handle the last few bytes of the input array
  switch (len) {
  case 3:
    h ^= data[2] << 16;
  case 2:
    h ^= data[1] << 8;
  case 1:
    h ^= data[0];
    h *= m;
  };

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  return h;
}

// calc string checksum
uint32 XStrUtil::check_sum(const char *value, uint32 length) {
  uint32 checksum = 0;
  for (int i = 0; i < length; i++) {
    checksum ^= value[i];
  }
  return checksum;
}

static char __dump_view(char ch) {
  if (ch <= 31 || ch >= 127) 
    return '.';
  return ch;
}

void XStrUtil::dump(std::string &result, const void *pdata, uint32 length) {
  result.clear();
  if (pdata == NULL || length == 0) {
    return;
  }

  /*
  char buf[128];
  const uint8 *src = (const uint8 *)pdata;
  for (; length >= 16; length -= 16, src += 16) {
    sprintf(buf,
            "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X "
            "%02X %02X %02X    ",
            src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7],
            src[8], src[9], src[10], src[11], src[12], src[13], src[14],
            src[15]);
    result += buf;

    sprintf(buf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\r\n", __dump_view(src[0]),
            __dump_view(src[1]), __dump_view(src[2]), __dump_view(src[3]),
            __dump_view(src[4]), __dump_view(src[5]), __dump_view(src[6]),
            __dump_view(src[7]), __dump_view(src[8]), __dump_view(src[9]),
            __dump_view(src[10]), __dump_view(src[11]), __dump_view(src[12]),
            __dump_view(src[13]), __dump_view(src[14]), __dump_view(src[15]));
    result += buf;
  }

  for (uint32 i = 0; i < length; i++) {
    sprintf(buf, "%02X ", src[i]);
    result += buf;
  }
  if (length % 16) result.append((16 - length) * 3 + 3, ' ');

  for (uint32 i = 0; i < length; i++) {
    result += __dump_view(src[i]);
  }
  if (length % 16) 
    result += "\r\n";
  */
  if ((NULL == pdata) || (0 == length)) {
    return;
  }

  for (uint32 i = 0; i < length; i++) {
    char buf[5] = {0};
    snprintf(buf, 4, "%02X ", (int)((char*)pdata)[i]);
    result += buf;
  }
  printf("------ %s\n", result.c_str());
  return;
}

const int32 MAX_RANDOM_KEY_SIZE = 62;
const char RANDOM_KEY[] =
  "abcdefghijkrmnopqlstuvwxyzABCDEFGHIJKRMNOPQLSTUVWXYZ0123456789";
std::string XStrUtil::random_string(std::size_t size) {
  std::string result;
  static bool seed_set = false;
  if (!seed_set) {
    srand(time(NULL));
    seed_set = true;
  }
  for (std::size_t i = 0; i < size; i++) {
    result.push_back(RANDOM_KEY[rand() % MAX_RANDOM_KEY_SIZE]);
  }
  return result;
}

char *XStrUtil::safe_strtok(char *str, const char *delim, char **saveptr) {
#ifdef WIN32
  return strtok_s(str, delim, saveptr);
#else
  return strtok_r(str, delim, saveptr);
#endif  // WIN32
}

void XStrUtil::safe_memcpy(void *p_dst, unsigned int dst_size,
                           const void *p_src, unsigned int src_size) {
  memcpy(p_dst, p_src, ((dst_size > src_size) ? src_size : dst_size));
}

static const char HEX[] = "0123456789abcdef";

char XStrUtil::hex_encode(unsigned char val) {
  ASSERT(val < 16);
  return (val < 16) ? HEX[val] : '!';
}

bool XStrUtil::hex_decode(char ch, unsigned char* val) {
  if ((ch >= '0') && (ch <= '9')) {
    *val = ch - '0';
  } else if ((ch >= 'A') && (ch <= 'Z')) {
    *val = (ch - 'A') + 10;
  } else if ((ch >= 'a') && (ch <= 'z')) {
    *val = (ch - 'a') + 10;
  } else {
    return false;
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////
static const unsigned char URL_UNSAFE = 0x1; // 0-33 "#$%&+,/:;<=>?@[\]^`{|} 127

//  ! " # $ % & ' ( ) * + , - . / 0 1 2 3 4 6 5 7 8 9 : ; < = > ?
//@ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [ \ ] ^ _
//` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~

static const unsigned char ASCII_CLASS[128] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,0,3,1,1,1,3,2,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,1,1,3,1,3,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1,
};

uint32 XStrUtil::url_encode(char* buffer, uint32 buflen,
                            const char* source, uint32 srclen) {
  if (NULL == buffer)
    return srclen * 3 + 1;
  if (buflen <= 0)
    return 0;

  uint32 srcpos = 0, bufpos = 0;
  while ((srcpos < srclen) && (bufpos + 1 < buflen)) {
    unsigned char ch = source[srcpos++];
    if ((ch < 128) && (ASCII_CLASS[ch] & URL_UNSAFE)) {
      if (bufpos + 3 >= buflen) {
        break;
      }
      buffer[bufpos + 0] = '%';
      buffer[bufpos + 1] = hex_encode((ch >> 4) & 0xF);
      buffer[bufpos + 2] = hex_encode((ch) & 0xF);
      bufpos += 3;
    } else {
      buffer[bufpos++] = ch;
    }
  }
  buffer[bufpos] = '\0';
  return bufpos;
}

uint32 XStrUtil::url_decode(char* buffer, uint32 buflen,
                            const char* source, uint32 srclen) {
  if (NULL == buffer)
    return srclen + 1;
  if (buflen <= 0)
    return 0;

  unsigned char h1, h2;
  uint32 srcpos = 0, bufpos = 0;
  while ((srcpos < srclen) && (bufpos + 1 < buflen)) {
    unsigned char ch = source[srcpos++];
    if (ch == '+') {
      buffer[bufpos++] = ' ';
    } else if ((ch == '%')
               && (srcpos + 1 < srclen)
               && hex_decode(source[srcpos], &h1)
               && hex_decode(source[srcpos + 1], &h2)) {
      buffer[bufpos++] = (h1 << 4) | h2;
      srcpos += 2;
    } else {
      buffer[bufpos++] = ch;
    }
  }
  buffer[bufpos] = '\0';
  return bufpos;
}

std::string XStrUtil::bin_2_hex(const std::string& src, bool upper) {
  std::string hex;
  hex.resize(src.size() * 2);
  for (size_t i = 0; i < src.size(); i++) {
    uint8_t cTemp = src[i];
    for (size_t j = 0; j < 2; j++) {
      uint8_t cCur = (cTemp & 0x0f);
      if (cCur < 10) {
        cCur += '0';
      } else {
        cCur += ((upper ? 'A' : 'a') - 10);
      }
      hex[2 * i + 1 - j] = cCur;
      cTemp >>= 4;
    }
  }

  return hex;
}

std::string XStrUtil::hex_2_bin(const std::string& src) {
  if (src.size() % 2 != 0) {
    return "";
  }

  std::string dst;
  dst.resize(src.size() / 2);
  for (size_t i = 0; i < dst.size(); i++) {
    uint8_t cTemp = 0;
    for (size_t j = 0; j < 2; j++) {
      char cCur = src[2 * i + j];
      if (cCur >= '0' && cCur <= '9') {
        cTemp = (cTemp << 4) + (cCur - '0');
      } else if (cCur >= 'a' && cCur <= 'f') {
        cTemp = (cTemp << 4) + (cCur - 'a' + 10);
      } else if (cCur >= 'A' && cCur <= 'F') {
        cTemp = (cTemp << 4) + (cCur - 'A' + 10);
      } else {
        return "";
      }
    }
    dst[i] = cTemp;
  }

  return dst;
}

}  // namespace chml
