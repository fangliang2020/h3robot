#ifndef FILECACHE_BASE_BASE_DEFINE_H_
#define FILECACHE_BASE_BASE_DEFINE_H_

#include "eventservice/mem/membuffer.h"

namespace cache {

#define ML_RECORD_PART_MAX            2   // 最大分区数量
#define ML_RECORD_PATH_SIZE           128
#define ML_RECORD_PATH                "RobotCap"  // 存储目录


typedef chml::MemBuffer  MemBuffer;

#define CACHE_ERROR_FILE_NOT_FOUND    1
#define CACHE_ERROR_TIMEOUT           2
#define CACHE_ERROR_NO_MEM            3
#define CACHE_DONE                    0

#define DEFAULT_REQ_TIMEOUT           3000

}


#endif  // FILECACHE_BASE_BASE_DEFINE_H_
