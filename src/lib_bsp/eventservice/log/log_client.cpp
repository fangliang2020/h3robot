#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(WIN32)
#include <comdef.h>
#include <windows.h>
#elif defined(POSIX)
#include <sys/prctl.h>
#include <sys/syscall.h>  // SYS_gettid
#include <sys/time.h>
#include <time.h>
#include <unistd.h>  // syscall
#endif
#include "eventservice/base/atomic.h"
#include "eventservice/base/byteorder.h"
#include "eventservice/base/nethelpers.h"

#include "eventservice/util/time_util.h"

#include "eventservice/event/thread.h"

#include "eventservice/log/log_cache.h"
#include "eventservice/log/log_client.h"
#include "eventservice/log/log_server.h"
#include "eventservice/vty/vty_client.h"

// 编译宏：格式化运行线程名称到Debug日志，liteos系统大概耗时10us，
// 其他系统开启，暂时关闭该功能
#if !defined(LITEOS)
#define LOG_FORMAT_THREAD_NAME
#endif

// 模块Debug日志配置信息
typedef struct {
  uint8 level;                     // 日志级别
  char name[LOG_MODULE_NAME_LEN];  // 模块名称
} LOG_DBG_MODULE_S;

// 日志模块实体
typedef struct {
  chml::ThreadManager *thread_mgr;

  bool sync_print_enable;         // 日志同步输出使能位
  LOG_PRINT_END_E dbg_print_end;  // Debug日志输出终端设备
  volatile long dbg_module_num;   // Debug日志模块数量
  LOG_DBG_MODULE_S dbg_module[LOG_MODULE_MAX_NUM];  // Debug日志模块配置信息
} LOG_CLIENT_ENTITY_S;

LOG_CLIENT_ENTITY_S *g_LogClient = NULL;

static char g_proc_name[64] = {0};

#ifdef LITEOS
#define LOG_CONTROL_THRESHOLD (1200)  // 1616 * 0.8，Debug日志流控门限
LOG_CACHE_CFG_S log_cache_cfg[] = {
  {32, 64},       // 2K
  {64, 256},      // 16K
  {128, 768},     // 96K
  {256, 512},     // 128K
  {1 * 1024, 8},  // 8K
  {2 * 1024, 4},  // 8K
  {4 * 1024, 4},  // 16K,total 274K
};
#elif defined WIN32
#define LOG_CONTROL_THRESHOLD (2200)  // 2928 * 0.8，Debug日志流控门限
LOG_CACHE_CFG_S log_cache_cfg[] = {
  {32, 256},       // 8K
  {64, 512},       // 32K
  {128, 4096},     // 512K
  {256, 4096},     // 1024k
  {1 * 1024, 64},  // 64K
  {2 * 1024, 32},  // 64K
  {4 * 1024, 16},  // 64K,total 1768k
};
#else                                 // linux
#ifdef CACHE_SIZE_LOW
#define LOG_CONTROL_THRESHOLD (1200)  // 1616 * 0.8，Debug日志流控门限
LOG_CACHE_CFG_S log_cache_cfg[] = {
  {32, 64},       // 2K
  {64, 256},      // 16K
  {128, 768},     // 96K
  {256, 512},     // 128K
  {1 * 1024, 8},  // 8K
  {2 * 1024, 4},  // 8K
  {4 * 1024, 4},  // 16K,total 274K
};
#else
#define LOG_CONTROL_THRESHOLD (2000)  // 2656 * 0.8，Debug日志流控门限
LOG_CACHE_CFG_S log_cache_cfg[] = {
  {32, 64},        // 2K
  {64, 512},       // 32K
  {128, 1024},     // 128K
  {256, 1024},     // 256K
  {1 * 1024, 16},  // 16K
  {2 * 1024, 8},   // 16K
  {4 * 1024, 8},   // 32K,total 482K
};
#endif
#endif

// 声明模块日志ID
unsigned int MOD_SS = MOD_EB;     // system_server
unsigned int MOD_AVS = MOD_EB;    // av_server
unsigned int MOD_EVT = MOD_EB;    // event_server
unsigned int MOD_BUS = MOD_EB;    // business
unsigned int MOD_TCP = MOD_EB;    // tcp_server
unsigned int MOD_O_TCP = MOD_EB;  // tcp_server_old
unsigned int MOD_WEB = MOD_EB;    // web_server
unsigned int MOD_HTTP = MOD_EB;   // http_sender
unsigned int MOD_XTP = MOD_EB;    // xtpush_service
unsigned int MOD_ONVIF = MOD_EB;  // onvif
unsigned int MOD_STP = MOD_EB;    // stp
unsigned int MOD_DG = MOD_EB;     // device_group
unsigned int MOD_ALG = MOD_EB;    // alg_proc
unsigned int MOD_PS = MOD_EB;     // playserver
unsigned int MOD_REC = MOD_EB;    // record
unsigned int MOD_BATCH = MOD_EB;  // batch
unsigned int MOD_ISP = MOD_EB;    // isp
unsigned int MOD_GVETC = MOD_EB;  // etc
unsigned int MOD_LV = MOD_EB;     // lv
unsigned int MOD_ONENET = MOD_EB;     // lv
unsigned int MOD_INTERACT = MOD_EB;  // interact_device
unsigned int MOD_SAL_MEDIA;       // sal media

static const char *MOD_NAME_SS = "ss";
static const char* MOD_NAME_PS = "ps";
static const char *MOD_NAME_AVS = "avs";
static const char* MOD_NAME_ALG = "alg";
static const char *MOD_NAME_EVT = "evt";
static const char* MOD_NAME_STP = "stp";
static const char *MOD_NAME_BUS = "bus";
static const char *MOD_NAME_WEB = "web";
static const char *MOD_NAME_TCP = "tcp";
static const char *MOD_NAME_TCP_O = "tcp_o";
static const char *MOD_NAME_HTTP  = "http";
static const char *MOD_NAME_ONVIF = "onvif";
static const char *MOD_NAME_DG    = "dg";
static const char* MOD_NAME_REC = "record";
static const char* MOD_NAME_BATCH = "batch";
static const char* MOD_NAME_ISP = "isp";
static const char* MOD_NAME_GVETC = "gvetc";
static const char* MOD_NAME_LV = "lv";
static const char* MOD_NAME_ONENET = "onenet";
static const char* MOD_NAME_INTERACT = "interact_device";
static const char* MOD_NAME_SAL_MEDIA = "sal_media";

static void Log_ModRegInit() {
  MOD_SS = Log_DbgModRegist(MOD_NAME_SS, LL_NONE);
  MOD_PS = Log_DbgModRegist(MOD_NAME_PS, LL_NONE);
  MOD_AVS = Log_DbgModRegist(MOD_NAME_AVS, LL_NONE);
  MOD_ALG = Log_DbgModRegist(MOD_NAME_ALG, LL_NONE);
  MOD_EVT = Log_DbgModRegist(MOD_NAME_EVT, LL_NONE);
  MOD_STP = Log_DbgModRegist(MOD_NAME_STP, LL_NONE);
  MOD_BUS = Log_DbgModRegist(MOD_NAME_BUS, LL_NONE);
  MOD_WEB = Log_DbgModRegist(MOD_NAME_WEB, LL_NONE);
  MOD_TCP = Log_DbgModRegist(MOD_NAME_TCP, LL_NONE);
  MOD_O_TCP = Log_DbgModRegist(MOD_NAME_TCP_O, LL_NONE);
  MOD_HTTP = Log_DbgModRegist(MOD_NAME_HTTP, LL_NONE);
  MOD_XTP = MOD_HTTP;
  MOD_ONVIF = Log_DbgModRegist(MOD_NAME_ONVIF, LL_NONE);
  MOD_DG = Log_DbgModRegist(MOD_NAME_DG,  LL_NONE);
  MOD_REC = Log_DbgModRegist(MOD_NAME_REC, LL_NONE);
  MOD_BATCH = Log_DbgModRegist(MOD_NAME_BATCH, LL_NONE);
  MOD_ISP = Log_DbgModRegist(MOD_NAME_ISP, LL_NONE);
  MOD_GVETC = Log_DbgModRegist(MOD_NAME_GVETC, LL_NONE);
  MOD_LV = Log_DbgModRegist(MOD_NAME_LV, LL_NONE);
  MOD_ONENET = Log_DbgModRegist(MOD_NAME_ONENET, LL_NONE);
  MOD_INTERACT = Log_DbgModRegist(MOD_NAME_INTERACT, LL_NONE);
  MOD_SAL_MEDIA = Log_DbgModRegist(MOD_NAME_SAL_MEDIA, LL_NONE);
}

void Log_DbgAllSetLevel(int level) {
  Log_DbgSetLevel(MOD_EB, (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_SS,     (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_AVS,    (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_EVT,    (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_BUS,    (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_TCP,    (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_O_TCP,  (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_WEB,    (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_HTTP,   (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_ONVIF,  (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_STP,    (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_DG,     (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_ALG,    (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_PS,     (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_BATCH,  (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_ISP,    (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_REC,    (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_LV,     (LOG_LEVEL_E)level);
  Log_DbgSetLevel(MOD_ONENET, (LOG_LEVEL_E)level);
}

// 原子添加LOG_NODE_S信息回调函数，添加时间信息
// pvMem:LOG_NODE_S指针
void AppendTime(void *pvMem) {
  LOG_NODE_S *cache = static_cast<LOG_NODE_S *>(pvMem);
  if (NULL == cache) {
    return;
  }

#ifdef WIN32
  // windows下无法获取精确的usec，每次递增1微秒来区分时间，
  // 但概率存在时间反转
  static uint32 offset = 0;
  offset = (offset + 1) & (31);
  cache->sec = (uint32)chml::XTimeUtil::TimeSecond();
  cache->usec = offset;
#else
  struct timeval tv = {0};
  gettimeofday(&tv, NULL);
  cache->sec = (uint32)tv.tv_sec;
  cache->usec = (uint32_t)tv.tv_usec;
#endif
}

// 发送日志
// type:日志类型
// level:日志级别，Debug日志有效, LOG_LEVEL_E
// data:日志数据buffer
// size:数据长度
// return 成功:true；失败:false
bool WriteCache(int type, int level, char *data, int size) {
  int len = size + sizeof(LOG_NODE_S);
  LOG_NODE_S *cache = static_cast<LOG_NODE_S *>(Log_CacheMalloc(len));
  if (NULL == cache) {
    // printf("[log]error:malloc cache buffer failed, size:%d\n", len);
    return false;
  }

  // memcpy(cache->body, data, size);
  memcpy(reinterpret_cast<uint8 *>(cache) + sizeof(LOG_NODE_S), data, size);
  cache->type = type;
  cache->level = level;
  cache->length = len;
  if (LOG_ERROR == Log_CacheAdd(cache)) {
    (void)Log_CacheFree(cache);
    return false;
  }
  return true;
}

void SyncPrintNode(LOG_NODE_S *node, char *time_s) {
#if defined(WIN32)
#define LPN_COLOR_RED (FOREGROUND_RED)
#define LPN_COLOR_GREEN (FOREGROUND_GREEN)
#define LPN_COLOR_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN)
#define LPN_COLOR_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

  switch (node->level) {
  case LL_ERROR:
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_INTENSITY | LPN_COLOR_RED);
    printf("%s\n", time_s);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_INTENSITY | LPN_COLOR_WHITE);
    break;
  case LL_WARNING:
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_INTENSITY | LPN_COLOR_YELLOW);
    printf("%s\n", time_s);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_INTENSITY | LPN_COLOR_WHITE);
    break;
  case LL_KEY:
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_INTENSITY | LPN_COLOR_GREEN);
    printf("%s\n", time_s);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_INTENSITY | LPN_COLOR_WHITE);
    break;
  case LL_NONE:
  default:
    printf("%s\n", time_s);
    break;
  }
#else
  switch (node->level) {
  case LL_ERROR:
    printf("\033[1;31;40m%s\033[0m\n", time_s);
    break;
  case LL_WARNING:
    printf("\033[1;33;40m%s\033[0m\n", time_s);
    break;
  case LL_KEY:
    printf("\033[1;32;40m%s\033[0m\n", time_s);
    break;
  case LL_NONE:
  default:
    printf("%s\n", time_s);
    break;
  }
#endif
}

void SyncReadyOutputString(LOG_NODE_S *node, char *data) {
  char time_s[1024] = {0};
  chml::TimeS tm;
  chml::XTimeUtil::TimeLocalMake(&tm, node->sec);
  const static char *kLogLevelString[] = {"DEBUG", "INFO", "WARNING",
                                          "ERROR", "KEY",  "-"
                                         };

  switch (node->level) {
  case LL_INFO:
  case LL_ERROR:
  case LL_WARNING:
  case LL_KEY:
  case LL_DEBUG:
    snprintf(time_s, 1020, "%04d-%02d-%02d %02d:%02d:%02d,%-6d,[%s]: %s",
             tm.year, tm.month, tm.day, tm.hour, tm.min, tm.sec, node->usec,
             kLogLevelString[node->level], data);
    break;
  case LL_NONE:
    snprintf(time_s, 1020, "%04d-%02d-%02d %02d:%02d:%02d,%-6d: %s", tm.year,
             tm.month, tm.day, tm.hour, tm.min, tm.sec, node->usec, data);
    break;
  }
  SyncPrintNode(node, time_s);
}

// DEBUG日志Client端打印
void SyncPrint(int type, int level, char *data, int size) {
  //将日志数据拼成一个字符串，然后输出
  // 建一个LOG信息节点头
  LOG_NODE_S cache[sizeof(LOG_NODE_S)];
  const int len = size + sizeof(LOG_NODE_S);
  AppendTime(cache);
  cache->type = type;
  cache->level = level;
  cache->length = len;
  SyncReadyOutputString(cache, data);
}

// Debug日志流控开启状态检查
// level: 日志级别,LOG_LEVEL_E
// return 开启:true；未开启:false
bool TraficControl(int level) {
  const int log_count = Log_CacheCnt();
  if ((LOG_CONTROL_THRESHOLD <= log_count) && (LL_WARNING > level)) {
    return true;
  }
  return false;
}

// 从文件全路径__FILE__截取文件名
// file: 文件全路径
inline char *CatFileName(char *file) {
  char *file_name;
#ifdef WIN32
  file_name = strrchr(file, '\\');
#else
  file_name = strrchr(file, '/');
#endif
  if (NULL == file_name) {
    file_name = file;
  } else {
    file_name += 1;
  }

  return file_name;
}

// 基础库MID注册，默认为第一个ID
void EventBaseReg(LOG_CLIENT_ENTITY_S *log_client) {
  // EventBase use the first ID: MOD_EB
  log_client->dbg_module[0].level = LL_INFO;
  snprintf(log_client->dbg_module[0].name, LOG_MODULE_NAME_LEN, "event_base");
  log_client->dbg_module_num++;
}

bool InitService(bool multi_process, bool cache_exist) {
  LOG_SRV_CONFIG_S config;
  Log_SrvConfigGet(&config);
  bool ret = Log_SrvInit(&config);
  if (!ret) {
    printf("[log]error:log service init failed\n");
    return false;
  }

  if (cache_exist) {
    ret = Log_CacheReset();
  } else {
    ret = Log_CacheCreate(log_cache_cfg,
                          sizeof(log_cache_cfg) / sizeof(LOG_CACHE_CFG_S),
                          multi_process);
  }
  if (!ret) {
    printf("[log]error:create log cache failed\n");
    return false;
  }
  return true;
}

#ifdef ENABLE_VTY
void VtySetLevel(const void *vty_data, const void *user_data, int argc,
                 const char *argv[]) {
  if (!g_LogClient) {
    Vty_Print(vty_data, "log module uninited\n");
    return;
  }
  if (argc < 3) {
    Vty_Print(vty_data, "invalid argc num:%d\n", argc);
    return;
  }

  const char *name = argv[1];
  int level = atoi(argv[2]);
  if ((LL_DEBUG > level) || (LL_NONE < level)) {
    Vty_Print(vty_data, "invalid level:%d\n", level);
    return;
  }

  int pos = -1;
  for (int i = 0; i < g_LogClient->dbg_module_num; i++) {
    if (0 == strcmp(name, g_LogClient->dbg_module[i].name)) {
      pos = i;
      break;
    }
  }
  if (-1 == pos) {
    Vty_Print(vty_data, "invalid module name:%s\n", name);
    return;
  } else {
    g_LogClient->dbg_module[pos].level = level;
    Vty_Print(vty_data, "set module %s level to %d successed\n", name, level);
  }
}

void VtySetALLLevel(const void* vty_data, const void* user_data, int argc,
                    const char* argv[]) {
  if (!g_LogClient) {
    Vty_Print(vty_data, "log module uninited\n");
    return;
  }
  if (argc < 2) {
    Vty_Print(vty_data, "invalid argc num:%d\n", argc);
    return;
  }

  int level = atoi(argv[1]);
  if ((LL_DEBUG > level) || (LL_NONE < level)) {
    Vty_Print(vty_data, "invalid level:%d\n", level);
    return;
  }

  for (int i = 0; i < g_LogClient->dbg_module_num; i++) {
    g_LogClient->dbg_module[i].level = level;
  }
  Vty_Print(vty_data, "set all level to %d successed\n", level);
}

void VtyDumpCacheInfo(const void *vty_data, const void *user_data, int argc,
                      const char *argv[]) {
  uint8 cache_count = 32;
  uint8 result = 0;
  uint32 total_size = 0;
  LOG_CACHE_INFO_S cache_info[32] = {0};
  result = Log_CacheInfo(cache_info, &cache_count);
  if (result > 0) {
    Vty_Print(vty_data, "log module level 1 cache usage:\n");
    Vty_Print(vty_data,
              "> stack | block size | block count | "
              "use count | max use count\n");
    for (int i = 0; i < result; i++) {
      Vty_Print(vty_data, "> %-2d | %-5d | %-5d | %-5d | %-5d \n", i,
                cache_info[i].usBlockSize, cache_info[i].usBlockCnt,
                cache_info[i].usBlockCntUsed, cache_info[i].usBlockCntUsedMax);
      total_size += cache_info[i].usBlockSize * cache_info[i].usBlockCnt;
    }
    Vty_Print(vty_data, "> total cache size %d(Byte)\n", total_size);
  }
}

void VtySetSyncPrint(const void *vty_data, const void *user_data, int argc,
                     const char *argv[]) {
  if (!g_LogClient) {
    Vty_Print(vty_data, "log module uninited\n");
    return;
  }
  if (argc < 2) {
    Vty_Print(vty_data, "invalid argc num:%d\n", argc);
    return;
  }
  bool on = atoi(argv[1]);
  bool res = Log_SetSyncPrint(on);
  if (res) {
    Vty_Print(vty_data, "set log sync print(on:%d) successed\n", on);
  } else {
    Vty_Print(vty_data, "set log sync print(on:%d) failed\n", on);
  }
}

void VtySetPrintEnd(const void *vty_data, const void *user_data, int argc,
                    const char *argv[]) {
  if (!g_LogClient) {
    Vty_Print(vty_data, "log module uninited\n");
    return;
  }
  if (argc < 3) {
    Vty_Print(vty_data, "invalid argc num:%d\n", argc);
    return;
  }
  int type = atoi(argv[1]);
  int print_end = atoi(argv[2]);
  bool res = Log_SetPrintEnd(type, print_end);
  if (res) {
    Vty_Print(vty_data, "set log type %d print end %d successed\n", type,
              print_end);
  } else {
    Vty_Print(vty_data, "set log type %d print end %d failed\n", type,
              print_end);
  }
}

void VtySetServer(const void *vty_data, const void *user_data, int argc,
                  const char *argv[]) {
  if (!g_LogClient) {
    Vty_Print(vty_data, "log module uninited\n");
    return;
  }
  if (argc < 3) {
    Vty_Print(vty_data, "invalid argc num:%d\n", argc);
    return;
  }

  in_addr addr;
  if (0 == chml::inet_pton(AF_INET, argv[1], &addr)) {
    Vty_Print(vty_data, "invalid ip address:%s\n", argv[1]);
    return;
  }
  uint32 ip = chml::NetworkToHost32(addr.s_addr);
  uint16 port = atoi(argv[2]);
  bool res = Log_SetRemoteServer(ip, port);
  Vty_Print(vty_data, "set log server(ip:%s, port:%d), res:%d\n", argv[1], port,
            res);
  (void)Log_SetPrintEnd(LT_DEBUG, LE_REMOTE);
}
void VtyAppPort(const void *vty_data, const void *user_data,
            int argc, const char *argv[]) {
  
  Vty_Print(vty_data, "dp server vtyport:%u\n", VTY_MOD_PORT_DP);
  Vty_Print(vty_data, "system server vtyport:%u\n", VTY_MOD_PORT_SS);
  Vty_Print(vty_data, "media server vtyport:%u\n", VTY_MOD_PORT_AVS);
  Vty_Print(vty_data, "alg server vtyport:%u\n", VTY_MOD_PORT_ALG);
  Vty_Print(vty_data, "event server vtyport:%u\n", VTY_MOD_PORT_EVT);
  Vty_Print(vty_data, "stp server vtyport:%u\n", VTY_MOD_PORT_STP);
  Vty_Print(vty_data, "bus server vtyport:%u\n", VTY_MOD_PORT_BUS);
  Vty_Print(vty_data, "tcp server vtyport:%u\n", VTY_MOD_PORT_TCP);
  Vty_Print(vty_data, "web server vtyport:%u\n", VTY_MOD_PORT_WEB);
  Vty_Print(vty_data, "http server vtyport:%u\n", VTY_MOD_PORT_HTTP);  
  Vty_Print(vty_data, "onvif server vtyport:%u\n", VTY_MOD_PORT_ONVIF);
  Vty_Print(vty_data, "record server vtyport:%u\n", VTY_MOD_PORT_REC);
  
}

void VtyRegistCmd(void) {
  (void)Vty_RegistCmd("chml_log_set_remote_server", (VtyCmdHandler)VtySetServer,
                      NULL,
                      "Set log remote servr address:"
                      "xx.xx.xx.xx, <1024-65535>");

  (void)Vty_RegistCmd("chml_log_set_sync_print", (VtyCmdHandler)VtySetSyncPrint,
                      NULL, "Set log sync print: [<0-1>]");

  (void)Vty_RegistCmd("chml_log_set_level", (VtyCmdHandler)VtySetLevel, NULL,
                      "set debug log level: "
                      "module name, LOG_LEVEL_E");

  (void)Vty_RegistCmd("chml_log_set_all_level", (VtyCmdHandler)VtySetALLLevel, NULL,
                      "set debug log level: "
                      "LOG_LEVEL_E");

  (void)Vty_RegistCmd("chml_log_set_print_end", (VtyCmdHandler)VtySetPrintEnd,
                      NULL, "set log print end: LOG_TYPE_E, LOG_PRINT_END_E");

  (void)Vty_RegistCmd("chml_log_dump_cache_info",
                      static_cast<VtyCmdHandler>(VtyDumpCacheInfo), NULL,
                      "print log cache usage info");
  (void)Vty_RegistCmd("chml_app_vtyport",
                      static_cast<VtyCmdHandler>(VtyAppPort), NULL,
                      "print app vty port");                   
}
#endif

void Log_HexDump(const char *discript, const unsigned char *data, uint32 size) {
  const uint8_t *c = data;
  if ((NULL == data) || (0 == size)) {
    return;
  }

  if (discript) {
    printf("%s,", discript);
  }

  printf("Dumping %u bytes from %p:\n", size, data);
  while (size > 0) {
    for (unsigned i = 0; i < 16; i++) {
      if (i < size)
        printf("%02x ", c[i]);
      else
        printf("   ");
    }

    for (unsigned i = 0; i < 16; i++) {
      if (i < size)
        printf("%c", c[i] >= 32 && c[i] < 127 ? c[i] : '.');
      else
        printf(" ");
    }
    printf("\n");

    c += 16;
    if (size <= 16)
      break;
    size -= 16;
  }
}

void Log_RelTraceMap2(int type, uint32 id_1, uint32 id_2, const char *format, ...) {
  if (!g_LogClient) {
    return;
  }

  char data[LOG_RELEASE_MAX_LEN + 1];
  // 可用长度，预留LOG_NODE_S 和 "\0"空间
  int limit = LOG_RELEASE_MAX_LEN - sizeof(LOG_NODE_S) - 1;
  int size = snprintf(data, limit, "%d,%d,", id_1, id_2);

  if (format) {
    va_list args;
    va_start(args, format);
    limit = limit - size;
    // Linux系统如果format长度超过源buffer的长度，则截断format，并返回
    // format的实际长度，字符串转换错误返回-1；Windows系统如果format长
    // 度超过源buffer的长度，则截断format，返回-1
    int ret = vsnprintf(data + size, limit, format, args);
    va_end(args);
    if (ret != -1) {
      size += ML_MIN(ret, limit);
    } else {
#ifdef WIN32
      size += limit;
#endif
    }
  }
  data[size] = '\0';
  size += 1;
  (void)WriteCache(type, LL_NONE, data, size);
}

bool Log_SetSyncPrint(bool on) {
  if (!g_LogClient) {
    printf("[log]error:log module uninited，SetSyncPrint failed\n");
    return false;
  }
  //if (on == g_LogClient->sync_print_enable) {
  //  return true;
  //}
  g_LogClient->sync_print_enable = on;
  printf("[logClient]the sync_print_enable is :%d\n",
         g_LogClient->sync_print_enable);
  return true;
}

void Log_RelTrace(int type, char *format, ...) {
  if (!g_LogClient) {
    return;
  }

  int size = 0;
  char data[LOG_RELEASE_MAX_LEN + 1];
  // 可用长度，预留LOG_NODE_S 和 "\0"空间
  int limit = LOG_RELEASE_MAX_LEN - sizeof(LOG_NODE_S) - 1;
  if (format) {
    va_list args;
    va_start(args, format);
    limit = limit - size;
    int ret = vsnprintf(data + size, limit, format, args);
    va_end(args);
    if (ret != -1) {
      size += ML_MIN(ret, limit);
    } else {
#ifdef WIN32
      size += limit;
#endif
    }
  }
  data[size] = '\0';
  size += 1;
#if 0
  if (g_LogClient->sync_print_enable) {
    LOG_SRV_FILE_TYPE_E fid;
    Log_SrvGetFileType(type, &fid);
    if (LogPrintEnd[fid] & LE_LOCAL) {
      SyncPrint(type, LL_NONE, data, size);
      return;
    }
  }
#endif
  (void)WriteCache(type, LL_NONE, data, size);
}

void Log_DbgTrace(int mod_id, int level, char *file, uint32 line, char *format,
                  ...) {
  if (!g_LogClient) {
    return;
  }

  uint32 pos = mod_id - LOG_MODULE_ID_BASE;
  if (pos >= (uint32)g_LogClient->dbg_module_num) {
    printf("[log]error:DbgTrace failed, invalid mid:%d, file: %s, line: %d\n",
           mod_id, file, line);
    return;
  }

  if ((level < g_LogClient->dbg_module[pos].level) || (level >= LL_NONE) ||
      (LE_OFF == g_LogClient->dbg_print_end)) {
    return;
  }

  // 流量控制
  if (TraficControl(level)) {
    static uint32 count = 0;
    if (0 == (count % 256)) {
      Log_DumpChacheInfo();
    }
    count++;
    return;
  }

  int size = 0, offset = 0;
  char data[LOG_DEBUG_MAX_LEN + 1];
  // 可用长度，预留LOG_NODE_S 和 "\0"空间
  int limit = LOG_DEBUG_MAX_LEN - sizeof(LOG_NODE_S) - 1;
#ifdef LOG_FORMAT_THREAD_NAME
  // 格式化运行线程名称
  bool known_name = false;
  if (g_LogClient->thread_mgr) {
    chml::Thread *thread = g_LogClient->thread_mgr->CurrentThread();
    if (thread) {
      const char *thread_name = thread->name().c_str();
      size = snprintf(data + offset, limit, "%s,", thread_name);
      limit -= size;
      offset += size;
      known_name = true;
    }
  }
  if (!known_name) {
#ifdef POSIX
    char thread_name[128] = {0};
    prctl(PR_GET_NAME, thread_name);
    size = snprintf(data + offset, limit, "%s,", thread_name);
#else
    size = snprintf(data + offset, limit, "%s,", "unnamed");
#endif
    limit -= size;
    offset += size;
  }
#endif

  char *file_name = CatFileName(file);
  size = snprintf(data + offset, limit, "%s,%d,", file_name, line);
  limit -= size;
  offset += size;
  if (format) {
    va_list args;
    va_start(args, format);
    size = vsnprintf(data + offset, limit, format, args);
    va_end(args);
    if (size != -1) {
      offset += ML_MIN(size, limit);
    } else {
#ifdef WIN32
      offset += limit;
#endif
    }
  }
  data[offset] = '\0';
  offset += 1;
  //如果开启同步打印
  if (g_LogClient->sync_print_enable) {
    if (g_LogClient->dbg_print_end & LE_LOCAL) {
      SyncPrint(LT_DEBUG, level, data, offset);
      return;
    }
  }
  (void)WriteCache(LT_DEBUG, level, data, offset);
}

bool Log_DbgSetLevel(int mod_id, int level) {
  if (!g_LogClient) {
    printf("[log]error:set log level failed(mid:%d, leve:%d)\n", mod_id, level);
    return false;
  }

  uint32 pos = mod_id - LOG_MODULE_ID_BASE;
  if (pos >= LOG_MODULE_MAX_NUM) {
    printf("[log]error:set log level failed, invalid mid:%d\n", mod_id);
    return false;
  }

  g_LogClient->dbg_module[pos].level = level;
  return true;
}

int Log_DbgModRegist(const char *name, int level) {
  if (!g_LogClient) {
    printf("[log]error:log module uninited\n");
    return -1;
  }

  if (!name || (0 == strlen(name))) {
    printf("[log]error:invalid module name\n");
    return -1;
  }

  do {
    // 查找是否已注册模块
    int pos = -1;
    for (int i = 0; i < g_LogClient->dbg_module_num; i++) {
      if (0 == strcmp(name, g_LogClient->dbg_module[i].name)) {
        pos = i;
        break;
      }
    }

    if (-1 == pos) {
      uint32 curr_num = ML_FAA(&(g_LogClient->dbg_module_num), 1);
      if ((curr_num + 1) > LOG_MODULE_MAX_NUM) {
        (void)ML_FAS(&(g_LogClient->dbg_module_num), 1);
        printf("[log]error:modules reach max num:%d, module name: %s\n",
               LOG_MODULE_MAX_NUM, name);
        break;
      }
      pos = curr_num;
      snprintf(g_LogClient->dbg_module[pos].name, LOG_MODULE_NAME_LEN, name);
    }

    g_LogClient->dbg_module[pos].level = level;
    return (pos + LOG_MODULE_ID_BASE);
  } while (0);

  printf("[log]error:module %s register failed\n", name);
  return -1;
}

bool Log_SetPrintEnd(int type, int print_end) {
  if (!g_LogClient) {
    printf("[log]error:log module uninited, Log_SetPrintEnd failed\n");
    return false;
  }

  if (LT_DEBUG == type) {
    g_LogClient->dbg_print_end = print_end;
  }
  return Log_SrvSetPrintEnd(type, print_end);
}

void Log_DumpChacheInfo() {
  uint8 cache_count = 32;
  uint8 result = 0;
  uint32 total_size = 0;
  LOG_CACHE_INFO_S cache_info[32] = {0};
  result = Log_CacheInfo(cache_info, &cache_count);
  if (result > 0) {
    printf("log module level 1 cache usage:\n");
    printf("> stack | block size | block count | use count | max use count\n");
    for (int i = 0; i < result; i++) {
      printf("> %-2d | %-5d | %-5d | %-5d | %-5d \n", i,
             cache_info[i].usBlockSize, cache_info[i].usBlockCnt,
             cache_info[i].usBlockCntUsed, cache_info[i].usBlockCntUsedMax);
      total_size += cache_info[i].usBlockSize * cache_info[i].usBlockCnt;
    }
    printf("> total cache size %d(Byte)\n", total_size);
  }
}

void Log_SetProcName(const char *name) {
#ifndef WIN32
  strncpy(g_proc_name, basename(name), sizeof(g_proc_name) - 1);
#else
  strncpy(g_proc_name, name, sizeof(g_proc_name)-1);
#endif
}

bool Log_ClientInit(void) {
#if defined(WIN32) || defined(LITEOS)
  return false;
#endif

  if (g_LogClient) {
    printf("[log]log client already inited\n");
    return true;
  }
  LOG_CLIENT_ENTITY_S *log_client_entity = NULL;

  do {
    const int size = sizeof(LOG_CLIENT_ENTITY_S);
    log_client_entity = static_cast<LOG_CLIENT_ENTITY_S *>(malloc(size));
    if (NULL == log_client_entity) {
      printf("[log]error:malloc log entity failed, size:%d\n", size);
      break;
    }
    memset(log_client_entity, 0x00, size);
    log_client_entity->sync_print_enable = false;
    log_client_entity->dbg_print_end = LE_LOCAL;
    EventBaseReg(log_client_entity);

    int res = Log_CacheCheck();
    if (0 != res) {
      printf("[log]error:not found log server process\n");
      break;
    }

    printf("[log]:log client init successed\n");
    g_LogClient = log_client_entity;

#ifdef ENABLE_VTY
    VtyRegistCmd();
#endif

    Log_ModRegInit();
    return true;
  } while (false);

  printf("[log]error:log client init failed\n");
  Log_Deinit();
  return false;
}

void Log_Deinit(void) {
  if (g_LogClient) {
    printf("[log]deinit log module\n");

    Log_CacheDestory();
    Log_SrvDeinit();

    free(g_LogClient);
    g_LogClient = NULL;
  }
}

bool Log_Init(bool multi_process) {
  if (g_LogClient) {
    printf("[log]log module already inited\n");
    return true;
  }
#ifdef MULTI_PROCESS
  multi_process = true;
#endif
  LOG_CLIENT_ENTITY_S *log_client_entity = NULL;
  // Windows暂不支持Server、Client模式，每个进程独立处理日志
#if defined(WIN32) || defined(LITEOS)
  multi_process = false;
#endif

  do {
    const int size = sizeof(LOG_CLIENT_ENTITY_S);
    log_client_entity = static_cast<LOG_CLIENT_ENTITY_S *>(malloc(size));
    if (NULL == log_client_entity) {
      printf("[log]error:malloc log entity failed, size:%d\n", size);
      break;
    }
    memset(log_client_entity, 0x00, size);
    log_client_entity->sync_print_enable = false;
    log_client_entity->dbg_print_end = LE_LOCAL;
    EventBaseReg(log_client_entity);

    bool result = true;
    if (multi_process) {
      int ret = Log_CacheCheck();
      if (-1 == ret) {
        result = InitService(true, false);
      } else if (-2 == ret) {
        result = InitService(true, true);
      }
    } else {
      result = InitService(false, false);
    }

    if (!result) {
      break;
    }
#ifdef LOG_FORMAT_THREAD_NAME
    log_client_entity->thread_mgr = chml::ThreadManager::Instance();
    if (NULL == log_client_entity->thread_mgr) {
      printf("[log]warning:get thread manager failed\n");
    }
#endif
#ifdef ENABLE_VTY
    VtyRegistCmd();
#endif
    printf("[log]:log module init successed\n");
    g_LogClient = log_client_entity;

    Log_ModRegInit();
    return true;
  } while (false);

  printf("[log]error:log module init failed\n");
  Log_Deinit();
  return false;
}
