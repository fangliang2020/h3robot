/**
 * @Description: 输出共享内存log
 * @Author:      zhz
 * @Version:     1
 */

#include <map>
#include <string>
#include <stdio.h>
#include <sys/shm.h>
#include "eventservice/base/atomic.h"
#include "eventservice/util/time_util.h"

enum { KEY_ID = 10010 };

// 队列数据dump类型
typedef enum {
  LOG_CACHE_DUMP_HEAD    = 0x01, // dump队列控制块数据
  LOG_CACHE_DUMP_IDX     = 0x02, // dump数据块索引数据
  LOG_CACHE_DUMP_OFFSET  = 0x04, // dump数据栈首地址
  LOG_CACHE_DUMP_DSTACK  = 0x08, // dump数据栈控制信息数据
  LOG_CACHE_DUMP_ALL     = 0x0F,
} LOG_CACHE_DUMP_TYPE_E;

// 日志类型
// - 枚举类型组合使用
typedef enum {
  // Debug日志类型
  LT_DEBUG = 0x01,  // 调试日志
  // Release日志类型
  LT_POINT = 0x02,  // 业务流程日志
  LT_SYS = 0x04,    // 系统日志
  LT_DEV = 0x08,    // 设备日志
  LT_UI = 0x10,     // 用户操作日志
  LT_USER1 = 0x20,
  LT_USER2 = 0x40,
  LT_USER3 = 0x80,
} LOG_TYPE_E_ITEM;
typedef unsigned char LOG_TYPE_E;

// Debug日志级别
typedef enum {
  LL_DEBUG,
  LL_INFO,
  LL_WARNING,
  LL_ERROR,
  LL_KEY,   // 用于关键流程日志打印，!!!该类型日志尽量少
  LL_NONE,  // 不输出任何级别的日志
} LOG_LEVEL_E;

// 内存栈索引，占高16位, 位移，低位为16或48位
#define MEM_STACK_TOP_OFFSET     ((sizeof(uint32_t) - sizeof(uint16_t)) << 3)
// 引用计数，占剩余低位, 掩码/
#define MEM_STACK_REF_MASK       (((uint32_t)1 << (MEM_STACK_TOP_OFFSET)) - 1)
// 内存栈尾节点next指针值，无效值/
#define MEM_STACK_NODE_INVALID   (0xFFFF)
// 数据块 尾部填充 长度
#define MEM_STACK_DATA_PADDING   (sizeof(uint32_t))
// 数据块 尾部填充 值
#define MEM_STACK_DATA_MAGIC     ((uint32_t)0x12345678)

// 索引队列中结点准备状态
typedef enum {
  NODE_EMPTY = 0,                  // 消息块初始状态
  NODE_READY,                      // 消息块已经准备好
  //NODE_INVALID,                    // 消息块处于无效状态
} IDX_QUEUE_NODE_STATE_E;

// 一级缓存节点信息
typedef struct {
  uint16_t length;  // 整条记录长度: LOG_NODE_S + body
  uint8_t type;     // 日志类型 (LOG_TYPE_E)
  uint8_t level;    // Debug日志级别，Release日志忽略
  uint32_t sec;     // 日志产生时间:秒
  uint32_t usec;    // 日志产生时间:微秒
  // uint8 body[0];  // 日志数据指针，存储结构为:trace data
} LOG_NODE_S;

// 内存栈节点信息结构体:该结构32位系统4字节对齐、64位系统均8字节对齐
//#define MEM_STACK_NODE_PACK (sizeof(uint32) - sizeof(long) - sizeof(long))
typedef struct {
  volatile long   usNext;          // 指向下一个数据块的索引号
  volatile uint32_t bUsedFlag;       // 标示该内存块是否在用
} MEM_STACK_NODE_S;

// 内存栈信息结构体:该结构32位系统4字节对齐、64位系统均8字节对齐
#define MEM_STACK_PACK (sizeof(uint32_t) - sizeof(uint16_t) - sizeof(uint16_t))
typedef struct {
  uint16_t          usDataBlockCnt;  // 内存栈中节点个数
  uint16_t          usDataBlockSize; // 内存栈中节点尺寸，用户配置的Size + 尾部Pad:MEM_STACK_DATA_MAGIC
#ifndef WIN32
  uint8_t           aucReserved1[MEM_STACK_PACK]; // 64位系统8字节对齐
#endif
  volatile uint32_t usUsedCnt;       // 内存栈中已经使用的节点个数
  volatile uint32_t usUsedCntMax;    // 内存栈中历史使用的节点最大个数
  volatile uint32_t ulTopAndRef;     // 高字16位-栈结构顶部结点序号,第一个可用的节点index;
  // 低16 or 48位-出栈次数，二者组合用于实现并发互斥
  MEM_STACK_NODE_S astNode[0];     // 队列存储区:索引数组
} MEM_STACK_S;

// 索引队列节点信息结构体:该结构32位系统4字节对齐、64位系统均8字节对齐
#define IDX_QUEUE_NODE_PACK (sizeof(uint32_t) - sizeof(uint16_t) - sizeof(uint8_t))
typedef struct {
  volatile long     ucState;         // 索引块的准备状态:NODE_EMPTY,NODE_READY
  uint16_t          usDataIdx;       // 内存栈中的节点索引号
  uint8_t           ucStackIdx;      // 内存栈索引号
  uint8_t           aucReserved[IDX_QUEUE_NODE_PACK];
} IDX_QUEUE_NODE_S;

// Log Cache实体、索引队列信息结构体
// 该结构32位系统4字节对齐、64位系统均8字节对齐
#define IDX_QUEUE_PACK_1 (sizeof(uint32_t) - sizeof(uint16_t) - sizeof(uint16_t))
#define IDX_QUEUE_PACK_2 (sizeof(uint32_t) - sizeof(uint8_t))
typedef struct {
  int             iServerMode;     // Server模式标识，Server模式下内存为SHM
  int             iShmId;          // Server模式SHM ID
  int             pid;             // 创建LogEntity进程的PID
  uint32_t          uiMemSize;       // Log Cache整个内存的大小
  uint16_t          usIdxBlockCnt;   // 索引队列数据块个数
  uint16_t          usIdxBlockSize;  // 索引队列数据块尺寸
#ifndef WIN32
  uint8_t           aucReserved1[IDX_QUEUE_PACK_1]; // 64位系统8字节对齐
#endif
  volatile uint32_t ulEnQCnt;        // 索引队列执行入队列次数，通过此来获取tail位置
  volatile uint32_t usHead;          // 索引队列中第一个可用结点索引
  volatile uint32_t usMsgCnt;        // 索引队列可用消息结点个数
  uint8_t           ucDStackCnt;     // 内存栈子栈的个数
  uint8_t           aucReserved[IDX_QUEUE_PACK_2];  // 占位，保证结构为4 or 8字节对接

  uint8_t           ucBody[0];       // 占位，mem stack偏移索引段 + 索引队列 + mem stack 1~n
  //uint32         aulOffset[0];    // mem stack偏移索引段起始地址，方便数据访问
} IDX_QUEUE_S;

IDX_QUEUE_S  *g_pstPQueue = NULL;
IDX_QUEUE_NODE_S *pstIdx = NULL;
void *pvDStackTop = NULL;
LOG_NODE_S *node = NULL;
std::string log_time, data;
std::map<int, std::string> level_map;
int level = 0;
bool filter = false;
std::string key;

// 获取索引队列首地址
// pstPQueue:Log Cache头指针
// return 索引队列首地址
static inline IDX_QUEUE_NODE_S* IdxQueueHead(IDX_QUEUE_S *pstPQueue) {
  return (IDX_QUEUE_NODE_S*)((uint32_t*)pstPQueue->ucBody + pstPQueue->ucDStackCnt);
  //return (IDX_QUEUE_NODE_S*)&pstPQueue->aulOffset[pstPQueue->ucDStackCnt];
}

// 索引队列节点地址
// pstHead:索引队列首地址
// usIndex:节点下标
// return 节点的地址
static inline IDX_QUEUE_NODE_S* IdxQueueNode(IDX_QUEUE_NODE_S *pstHead,
    uint16_t uiIndex) {
  return &pstHead[uiIndex];
}

// 根据下标取内存栈的偏移
// pstPQueue:Log Cache头指针
// usIndex:内存栈索引
// return 内存栈的偏移量
static inline uint32_t MemStackOffset(IDX_QUEUE_S *pstPQueue,
                                    uint16_t usIndex) {
  return *(((uint32_t*)pstPQueue->ucBody) + usIndex);
  //return pstPQueue->aulOffset[usIndex];
}

// 根据下标取内存栈首地址
// pstPQueue:Log Cache头指针
// usIndex:内存栈索引
// return 内存栈首地址
static inline MEM_STACK_S* MemStackHead(IDX_QUEUE_S *pstPQueue,
                                        uint16_t usIndex) {
  return (MEM_STACK_S*)&pstPQueue->ucBody[MemStackOffset(pstPQueue, usIndex)];
}

// 根据下标取内存栈节点地址
// pstStack:内存栈指针
// usIndex:栈节点下标
// return 节点地址
static inline MEM_STACK_NODE_S* MemStackNode(MEM_STACK_S *pstStack,
    uint32_t usIndex) {
  return &pstStack->astNode[usIndex];
}

// 根据下标取内存栈节点数据区首地址
// pstStack:内存栈指针
// usIndex:栈节点下标
// return 节点数据区首地址
static inline void* MemStackData(MEM_STACK_S *pstStack, uint32_t usIndex) {
  uint8_t *pucData = (uint8_t*)&pstStack->astNode[pstStack->usDataBlockCnt];
  return &pucData[pstStack->usDataBlockSize * usIndex];
}

// 取栈数据区总长
// pstStack:内存栈指针
// return 数据区总长
static inline uint32_t MemStackSize(MEM_STACK_S *pstStack) {
  return pstStack->usDataBlockCnt * pstStack->usDataBlockSize;
}

// 检查内存是否覆盖 Padding填充区
// pstStack:内存栈指针
// usIndex:内存块下标
// pvData:内存块首地址
static void MemStackNodeCheck(MEM_STACK_S *pstStack, uint16_t usIndex,
                              void* pvData) {
  uint8_t  *pucData = (uint8_t*)pvData;
  uint32_t  *pulPad  = NULL;

  pulPad = (uint32_t*)&pucData[pstStack->usDataBlockSize - MEM_STACK_DATA_PADDING];
  if (*pulPad == MEM_STACK_DATA_MAGIC) {
    return;
  }

  printf("[logCache]error: pvData:%p, corrupts pad!!!\n", pvData);
}

void Log_CacheDump(uint8_t ucType, int16_t sDStackIdx) {
  uint8_t   *pucBuf  = NULL;
  uint32_t   uiSize  = 1024;
  int      iOffset = 0;
  uint16_t   i       = 0;

  if (NULL == g_pstPQueue) {
    return;
  }

  pucBuf = (uint8_t*)malloc(uiSize);
  if (NULL == pucBuf) {
    return;
  }

  // dump queue header info
  if (ucType & LOG_CACHE_DUMP_HEAD) {
    printf("PQueue(%p): \r\n\tidx-cnt:%d \r\n\tidx-size:%d \r\n\thead:%d \r\n\t"
           "tail:%lu \r\n\tenqcnt:%lu \r\n\tmsg-cnt:%d \r\n\tds-cnt:%d\n",
           g_pstPQueue,
           g_pstPQueue->usIdxBlockCnt,
           g_pstPQueue->usIdxBlockSize,
           g_pstPQueue->usHead,
           g_pstPQueue->ulEnQCnt % g_pstPQueue->usIdxBlockCnt,
           g_pstPQueue->ulEnQCnt,
           g_pstPQueue->usMsgCnt,
           g_pstPQueue->ucDStackCnt);
  }

  // dump data stack address
  if (ucType & LOG_CACHE_DUMP_OFFSET) {
    iOffset = snprintf((char*)pucBuf, uiSize,
                       "PQueue-Offsets(%p):\r\n\t", g_pstPQueue->ucBody);
    for (i=0; i<g_pstPQueue->ucDStackCnt; i++) {
      iOffset += snprintf((char*)pucBuf + iOffset, uiSize - iOffset, "0x%x ",
                          MemStackOffset(g_pstPQueue, i));
    }
    printf("%s", pucBuf);
  }

  // dump index array info
  if (ucType & LOG_CACHE_DUMP_IDX) {
    IDX_QUEUE_NODE_S  *pstHdr;
    pstHdr  = IdxQueueHead(g_pstPQueue);
    iOffset = snprintf((char*)pucBuf, uiSize, "PQueue-Idx(%p):\r\n\t", pstHdr);
    for (i=0; i<g_pstPQueue->usIdxBlockCnt; i++) {
      IDX_QUEUE_NODE_S  *pstIdx;
      pstIdx = IdxQueueNode(pstHdr, i);
      iOffset += snprintf((char*)pucBuf + iOffset, uiSize - iOffset,
                          "stack-idx:%d, blk-idx:%d, state:%d",
                          pstIdx->ucStackIdx, pstIdx->usDataIdx, (int)pstIdx->ucState);
      if (((i+1) % 25) != 0) {
        iOffset += snprintf((char*)pucBuf + iOffset, uiSize - iOffset, "\r\n\t");
        continue;
      }

      printf("%s", pucBuf);
      pucBuf[0] = '\t';
      pucBuf[1] = '\0';
      iOffset = 1;
    }

    if (((i+1) % 25) != 0) {
      *(pucBuf + iOffset - 3) = '\0';
      printf("%s", pucBuf);
    }
  }

  // dump all data stack header info or the stack header which id is ucPQueueDIdx
  if (ucType & LOG_CACHE_DUMP_DSTACK) {
    MEM_STACK_S  *pstDStack;
    void         *pvDStackTop;
    if (-1 == sDStackIdx) {
      iOffset = 0;
      // data stack infos
      for (i=0; i<g_pstPQueue->ucDStackCnt; i++) {
        iOffset  += snprintf((char*)pucBuf + iOffset, uiSize - iOffset,
                             "DStack%d(%p): \r\n\tblk-cnt:%d, blk-size:%d, "
                             "topandref:%lu, use-cnt:%d, use-max:%d",
                             i,
                             pstDStack,
                             pstDStack->usDataBlockCnt,
                             pstDStack->usDataBlockSize,
                             pstDStack->ulTopAndRef,
                             pstDStack->usUsedCnt,
                             pstDStack->usUsedCntMax);
        if (((i+1) % 15) != 0) {
          iOffset += snprintf((char*)pucBuf + iOffset, uiSize - iOffset, "\r\n");
          continue;
        }

        printf("%s\n", pucBuf);
        pucBuf[0] = '\t';
        pucBuf[1] = '\0';
        iOffset = 1;
      }

      if (((i+1) % 15) != 0) {
        *(pucBuf + iOffset - 2) = '\0';
        printf("%s", pucBuf);
      }
    } else {
      if ((0 <= sDStackIdx) && (sDStackIdx < g_pstPQueue->ucDStackCnt)) {
        pstDStack = MemStackHead(g_pstPQueue, sDStackIdx);
        printf("DStack%d(%p): \r\n\tblk-cnt:%d, blk-size:%d, topandref:%lu, "
               "use-cnt:%d, use-max:%d\n",
               i,
               pstDStack,
               pstDStack->usDataBlockCnt,
               pstDStack->usDataBlockSize,
               pstDStack->ulTopAndRef,
               pstDStack->usUsedCnt,
               pstDStack->usUsedCntMax);
      }
    }
  }

  if (pucBuf) {
    free(pucBuf);
  }
}

int my_printf(int cur) {
  pstIdx = IdxQueueNode(IdxQueueHead(g_pstPQueue), cur);
  pvDStackTop = MemStackData(MemStackHead(g_pstPQueue, pstIdx->ucStackIdx), pstIdx->usDataIdx);
  node = (LOG_NODE_S *)pvDStackTop;
  log_time = chml::XTimeUtil::TimeLocalToString(node->sec);
  data = (char *)pvDStackTop + sizeof(LOG_NODE_S);
  if (!filter || (filter && data.find(key) != data.npos)) {
    if (LT_DEBUG == node->type) {
      if (node->level >= level) {
        switch (node->level) {
          case LL_ERROR:
            printf("\033[1;31;40m%s [%s] %s\033[0m\n", log_time.c_str(), level_map[LL_ERROR].c_str(), data.c_str());
            break;
          case LL_WARNING:
            printf("\033[1;33;40m%s [%s] %s\033[0m\n", log_time.c_str(), level_map[LL_WARNING].c_str(), data.c_str());
            break;
          case LL_KEY:
            printf("\033[1;32;40m%s [%s] %s\033[0m\n", log_time.c_str(), level_map[LL_KEY].c_str(), data.c_str());
            break;
          case LL_NONE:
          default:
            printf("%s [%s] %s\n", log_time.c_str(), level_map[node->level].c_str(), data.c_str());
            break;
        }
      }
    } else if (node->type > LT_DEBUG) {
      printf("\033[1;31;40m%s [POINT] %s\033[0m\n", log_time.c_str(), data.c_str());
    }
  }
  
  return 0;
}

int main(int argc, char *argv[]) {
  level_map[LL_DEBUG] = "DEBUG";
  level_map[LL_INFO] = "INFO";
  level_map[LL_WARNING] = "WARNING";
  level_map[LL_ERROR] = "ERROR";
  level_map[LL_KEY] = "KEY";
  level_map[LL_NONE] = "NONE";

  if (argc == 2) {
    level = std::stoi(argv[1]);
    printf("level %d\n", level);
  } else if (argc == 3) {
    filter = true;
    level = std::stoi(argv[1]);
    key = argv[2];
    printf("level %d key %s\n", level, key.c_str());
  }

  int cur = 0, tail = 0, blk_cnt = 0;
  int log_shmid = -1;
  log_shmid = shmget(KEY_ID, 0, 0);
  if (log_shmid == -1) {
    printf("shmget fail!\n");
    return 0;
  }

  g_pstPQueue = (IDX_QUEUE_S *)shmat(log_shmid, NULL, SHM_RDONLY);
  if (g_pstPQueue == NULL) {
    printf("shmat fail!\n");
    return 0;
  }

  blk_cnt = g_pstPQueue->usIdxBlockCnt;
  cur = g_pstPQueue->ulEnQCnt % g_pstPQueue->usIdxBlockCnt;

  do {
    tail = g_pstPQueue->ulEnQCnt % g_pstPQueue->usIdxBlockCnt;
    if (cur == tail) {
      usleep(100);
      continue;
    } else if (tail > cur) {
      for (; cur < tail; ++cur) {
        my_printf(cur);
      }
    } else {
      for (; cur < blk_cnt; ++cur) {
        my_printf(cur);
      }
      cur = 0;
      for (; cur < tail; ++cur) {
        my_printf(cur);
      }
    }
  } while (1);

  shmdt(g_pstPQueue);

  return 0;
}
