#ifndef _ML_CRC32_MY_H_
#define _ML_CRC32_MY_H_
#include "eventservice/base/basictypes.h"

namespace chml {
class MyCrc32{
  public:
   static unsigned int crc32_my(unsigned int crc, const char *buf, unsigned int len);
};
}
#endif