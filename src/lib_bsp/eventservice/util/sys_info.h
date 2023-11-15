#ifndef SRC_BASE_SYSINFO_H_
#define SRC_BASE_SYSINFO_H_

#include <stdint.h>

typedef struct {
  uint32_t ts;
  uint32_t freed;
  uint32_t cached;
} meminfo_t;


bool GetMemInfo(meminfo_t *meminfo);

uint32_t UsrAppMemUsed();


#endif