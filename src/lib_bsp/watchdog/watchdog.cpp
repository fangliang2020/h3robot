#include "watchdog.h"

#include <cstdio>
#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>  // For mode constants
#include <sys/types.h>
#if defined(WIN32)
#include <windows.h>
#elif defined(POSIX)
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/shm.h>
#endif

#include "eventservice/base/atomic.h"

#include "eventservice/base/basictypes.h"

#include "eventservice/util/file_opera.h"
#include "eventservice/util/time_util.h"
#include "eventservice/util/string_util.h"

#include "shmc/svipc_shm_alloc.h"
#include "eventservice/util/sys_info.h"

#include "eventservice/log/log_client.h"
#include "eventservice/vty/vty_client.h"

#define WDG_MODULE_NAME_LEN     (32)        // 模块注册的最大名称长度
#define WDG_MODULE_MAX_NUM      (64)        // 最大可注册的模块数量
#define WDG_ENTITY_SHM_KEY      "0x10011"   // watchdog共享内存Key

#include <stdlib.h>

#include "eventservice/net/eventservice.h"

#include "eventservice/event/tls.h"
#include "eventservice/event/messagehandler.h"

namespace wdg {

class CModuleWatchdog {
 private:
  CModuleWatchdog() : watchdog_(nullptr) {}

 public:
  ~CModuleWatchdog() {
    watchdog_ = nullptr;
  }

  static CModuleWatchdog *Instance(bool breset = false) {
    static CModuleWatchdog *pthiz = NULL;
    if (NULL == pthiz) {
      pthiz = new CModuleWatchdog();
      if (pthiz) {
        int32_t nret = pthiz->Init(breset);
        if (0 == nret) {
        } else {
          DLOG_ERROR(MOD_EB, "watchdog init failed. %d.\n", nret);
        }
      } else {
        DLOG_ERROR(MOD_EB, "watchdog create failed.\n");
      }
    }
    return pthiz;
  }

 public:
  // 模块注册
  int32_t Register(const std::string &mod_name, uint32_t mod_timeout) {
    if (nullptr == watchdog_) {
      DLOG_ERROR(MOD_EB, "watchdog is not init.\n");
      return -1;
    }

    if (mod_timeout < WDG_MIN_TIMEOUT) {
      mod_timeout = WDG_DEF_TIMEOUT;
    }

    shmc::SVIPCScope cs(&wdg_mutex_);

    int32_t i = 0;
    // 查找是否已注册
    for (i = 0; i < WDG_MODULE_MAX_NUM; i++) {
      int32_t n = chml::XStrUtil::compare(mod_name, watchdog_->modules[i].name);
      if (0 == n) {
        DLOG_WARNING(MOD_EB, "watchdog was register.\n");
        return i;
      }
    }

    // 查找空闲module空间进行注册
    for (i = 0; i < WDG_MODULE_MAX_NUM; i++) {
      if (1 == watchdog_->modules[i].registered) {
        continue;
      }

      memset(&watchdog_->modules[i], 0, sizeof(TAG_MODULE));
      watchdog_->modules[i].registered = 1;
      watchdog_->modules[i].timeout = mod_timeout;
      watchdog_->modules[i].last_feed_time = chml::XTimeUtil::ClockSecond();
      chml::XStrUtil::safe_memcpy(watchdog_->modules[i].name, WDG_MODULE_NAME_LEN, mod_name.c_str(), mod_name.size());
      break;
    }
    // 无空闲module空间
    if (i == WDG_MODULE_MAX_NUM) {
      DLOG_ERROR(MOD_EB, "there is not free module.\n");
      return -1;
    }

    DLOG_INFO(MOD_EB, "watchdog %s is register %u.\n", watchdog_->modules[i].name, i);
    return i;
  }

  int32_t SetTimeout(int key, uint32_t mod_timeout) {
    if (nullptr == watchdog_) {
      DLOG_WARNING(MOD_EB, "watchdog is not init.\n");
      return -1;
    }

    if (key < 0 || key >= WDG_MODULE_MAX_NUM) {
      DLOG_WARNING(MOD_EB, "invalid module key: %d, out of range: [0, %d).\n", key, WDG_MODULE_MAX_NUM);
      return -1;
    }

    if (mod_timeout < WDG_MIN_TIMEOUT) {
      mod_timeout = WDG_DEF_TIMEOUT;
    }

    shmc::SVIPCScope cs(&wdg_mutex_);

    if (watchdog_->modules[key].registered) {
      watchdog_->modules[key].timeout = mod_timeout;
    } else {
      DLOG_WARNING(MOD_EB, "module key %d is not register\n", key);
      return -1;
    }

    return 0;
  }

  int32_t Feeddog(int32_t mod_num) {
    if (nullptr == watchdog_) {
      DLOG_ERROR(MOD_EB, "watchdog is not init.\n");
      return -1;
    }

    if (mod_num < 0 || mod_num >= WDG_MODULE_MAX_NUM) {
      DLOG_ERROR(MOD_EB, "invalid module key: %d, out of range: [0, %d).\n", mod_num, WDG_MODULE_MAX_NUM);
      return -1;
    }

    // if memory is reseted, need update buffer
    if (0 == watchdog_->modules[mod_num].registered) {
      DLOG_WARNING(MOD_EB, "mod: %d not register.\n", mod_num);
    }

    // update watchdog time
    // DLOG_INFO(MOD_EB, "%s feeddog.\n", module_.name);
    watchdog_->modules[mod_num].last_feed_time = chml::XTimeUtil::ClockSecond();
    return 0;
  }

  int Enable(int enable) {
    if (nullptr == watchdog_) {
      DLOG_ERROR(MOD_EB, "watchdog is not init.\n");
      return -1;
    }
    shmc::SVIPCScope cs(&wdg_mutex_);
    watchdog_->enable = (enable == 0 ? false : true);
    DLOG_ERROR(MOD_EB, "watchdog enable %d.\n", watchdog_->enable);
    return 0;
  }

 protected:
  // create | attach share memory
  int32_t Init(bool breset = false) {
    std::string key = WDG_ENTITY_SHM_KEY;

    // create share memory
    void *pshm = wdg_shm_.Attach_AutoCreate(key + "0", TAG_WATCHDOG_SIZE);
    if (nullptr == pshm) {
      DLOG_ERROR(MOD_EB, "create share memory failed.\n");
      return -1;
    }

    // create memory mutex
    if (0 != wdg_mutex_.Init(key + "1")) {
      DLOG_ERROR(MOD_EB, "create share memory mutex failed.\n");
      return -2;
    }
    DLOG_INFO(MOD_EB, "create share memory & mutex success.\n");
    //
    watchdog_ = (TAG_WATCHDOG*)pshm;

    // reset watchdog memory
    if (breset) {
      memset(pshm, 0, TAG_WATCHDOG_SIZE);

      watchdog_->enable = true;
      for (int32_t i = 0; i < WDG_MODULE_MAX_NUM; i++) {
        watchdog_->modules[i].registered = 0;
        watchdog_->modules[i].last_feed_time = 0;
        watchdog_->modules[i].timeout = WDG_DEF_TIMEOUT;
      }

      // read default config
      OnDefaultConfig();
    }
    return 0;
  }

 public:
  // 检查模块喂狗状况
  bool OnCheckModule() {
    bool is_timeout = false;

    shmc::SVIPCScope cs(&wdg_mutex_);

    if (!watchdog_ || !watchdog_->enable) {
      DLOG_ERROR(MOD_EB, "watchdog_:%p, enable:%s\n", watchdog_, (watchdog_ && watchdog_->enable ? "true" : "false"));
      return is_timeout;
    }

    int32_t i = 0;
    uint32_t now_sys_clk = chml::XTimeUtil::ClockSecond();
    for (i = 0; i < WDG_MODULE_MAX_NUM; i++) {
      if (watchdog_ && watchdog_->modules[i].registered) {
        uint32_t ndiff = now_sys_clk - watchdog_->modules[i].last_feed_time;

        
        if (ndiff > watchdog_->modules[i].timeout) {
          char line[1024];
          meminfo_t meminfo;
          GetMemInfo(&meminfo);
          snprintf(line, 1024, "%04d timeout %s, free:%u, cached:%u, usrapp mem:%u", 
                  LOG_ID_WATCHDOG_REBOOT, watchdog_->modules[i].name, meminfo.freed,
                  meminfo.cached, UsrAppMemUsed());

          LOG_TRACE(LT_SYS, line);
          DLOG_ERROR(MOD_EB, "%s\n", line);
          is_timeout = true;
          const char *wdg_path = "/mnt/log/system_server_files/wdg.txt";
          FILE *fp = fopen(wdg_path, "w");
          if(fp != NULL) {
            fwrite(line, strlen(line), 1, fp);
            fflush(fp);
            fclose(fp);
          }
        }
      }
    }

    if (is_timeout) {
      OnTimeouted();
    }
    return is_timeout;
  }

  // 处理模块超时
  void OnTimeouted() {
    // 先记录日志，并刷新日志到文件中，然后重启系统
    const static uint32_t MAX_LOG_LEN = 2048;  // 确保足够长，不会溢出
    int size_nor = 0, size_out = 0;
    int offset_nor = 0, offset_out = 0;
    int limit_nor = MAX_LOG_LEN - 1, limit_out = MAX_LOG_LEN - 1;
    char data_nor[MAX_LOG_LEN] = { 0 };
    char data_out[MAX_LOG_LEN] = { 0 };

    size_out = snprintf(data_out, limit_out, "wdg timeout mod:");
    limit_out -= size_out;
    offset_out += size_out;
    size_nor = snprintf(data_nor, limit_nor, "wdg normal mod:");
    limit_nor -= size_nor;
    offset_nor += size_nor;
    DLOG_KEY(MOD_EB, "Watch dog timeout, will reboot system now");

    uint32_t now_sys_clk = chml::XTimeUtil::ClockSecond();
    for (int32_t i = 0; i < WDG_MODULE_MAX_NUM; i++) {
      if (0 == watchdog_->modules[i].registered) {
        continue;
      }

      uint32_t ndiff = now_sys_clk - watchdog_->modules[i].last_feed_time;
      if (ndiff > watchdog_->modules[i].timeout) {
        size_out = snprintf(data_out + offset_out, limit_out, "%.*s[%d,%d],", 5,
                            watchdog_->modules[i].name,
                            watchdog_->modules[i].timeout,
                            ndiff);
        limit_out -= size_out;
        offset_out += size_out;
      } else {
        size_nor = snprintf(data_nor + offset_nor, limit_nor, "%.*s[%d,%d],", 5,
                            watchdog_->modules[i].name,
                            watchdog_->modules[i].timeout,
                            ndiff);
        limit_nor -= size_nor;
        offset_nor += size_nor;
      }
    }

    data_out[offset_out] = '\0';
    data_nor[offset_nor] = '\0';
#if 1
    LOG_TRACE(LT_SYS, "%04d %s", LOG_ID_WATCHDOG_REBOOT, data_out);
    LOG_TRACE(LT_SYS, "%04d %s", LOG_ID_WATCHDOG_REBOOT, data_nor);

    Log_FlushCache();
#endif
    DLOG_DEBUG(MOD_EB, "\n[wdg]error: %s\n", data_out);
    DLOG_DEBUG(MOD_EB, "\n[wdg]info: %s\n", data_nor);
  }

  // TODO: read default config
  void OnDefaultConfig() {
    DLOG_DEBUG(MOD_EB, "Default config file %s loaded\n", WDG_DEF_MODULE_FILE);

    chml::XFile file;
    bool bret = file.open(WDG_DEF_MODULE_FILE, chml::IO_MODE_RD_ONLY);
    if (false == bret) {
      DLOG_ERROR(MOD_EB, "open file %s failed, errno:%s\n", WDG_DEF_MODULE_FILE,
                 strerror(errno));
      return;
    }

    int32_t num = 0;
    std::string str_line;
    TAG_MODULE* pmods = watchdog_->modules;
    do {
      file.read_line(str_line);
      if (str_line.empty() || '#' == str_line.c_str()[0]) {
        DLOG_ERROR(MOD_EB, "this line is %s.\n", str_line.c_str());
        continue;
      }

      std::size_t npos = str_line.find(';');
      if (npos == str_line.npos) {
        DLOG_ERROR(MOD_EB, "this line have no ';' char.\n");
        continue;
      }

      std::string mod_name = str_line.substr(0, npos);

      int32_t ntimeout = 0;
      chml::XStrUtil::to_int(str_line.substr(npos + 1), ntimeout);
      if (mod_name.size() < WDG_MODULE_NAME_LEN && ntimeout > 0) {
        pmods[num].registered = 1;
        pmods[num].timeout = (uint32_t)ntimeout;
        pmods[num].last_feed_time = chml::XTimeUtil::ClockSecond();
        chml::XStrUtil::safe_memcpy(pmods[num].name, WDG_MODULE_NAME_LEN,
                                    mod_name.c_str(), mod_name.size());
        num++;
      }
    } while (false == file.is_end());

    // DLOG_ERROR(MOD_EB, "load defult cfg successed, total %d modules", g_WdgEntity->module_num);
  }

 private:
  typedef struct _TAG_MODULE {
    char            name[WDG_MODULE_NAME_LEN];  // 模块名称
    uint32_t        registered;                 // 注册标识
    uint32_t        timeout;                    // 超时时间
    volatile uint32 last_feed_time;             // 上次喂狗时间
  } TAG_MODULE;

  typedef struct _TAG_WATCHDOG {
    bool             enable;
    TAG_MODULE       modules[WDG_MODULE_MAX_NUM];
  } TAG_WATCHDOG;
  static const uint32_t TAG_WATCHDOG_SIZE = sizeof(TAG_WATCHDOG);

  shmc::SVIPC       wdg_shm_;       // 共享内存
  shmc::SVIPCMutex  wdg_mutex_;     // 共享内存锁

  TAG_WATCHDOG*     watchdog_;      // 共享内存存储的看门狗信息
};

}  // namespace wdg

/* 服务端 */
// 看门狗模块初始化
// return 成功: 0, 失败: -1
int WatchDog_Init() {
  if (nullptr != wdg::CModuleWatchdog::Instance(true)) {
    return 0;
  }
  return -1;
}

// 看门狗模块检查模块
// return 超时: 1; 未超时 0
int WatchDog_CheckModule(void) {
  if (wdg::CModuleWatchdog::Instance()
      && wdg::CModuleWatchdog::Instance()->OnCheckModule()) {
    return 1;
  }
  return 0;
}

/* 客户端 */
// [子进程使用]看门狗模块初始化
int WatchDog_InitSubProc(void) {
  if (nullptr != wdg::CModuleWatchdog::Instance()) {
    return 0;
  }
  return -1;
}

// 看门狗模块注册,对于已注册的模块可通过该接口修改超时时间，
int WatchDog_Register(const char* name, unsigned int sec_timeout) {
  if (wdg::CModuleWatchdog::Instance()) {
    return wdg::CModuleWatchdog::Instance()->Register(name, sec_timeout);
  }
  return -1;
}

int WatchDog_SetTimeout(int key, unsigned int sec_timeout) {
  if (wdg::CModuleWatchdog::Instance()) {
    return wdg::CModuleWatchdog::Instance()->SetTimeout(key, sec_timeout);
  }
  return -1;
}

// 模块喂狗,支持多线程并发
int WatchDog_FeedDog(int key) {
  if (wdg::CModuleWatchdog::Instance()) {
    return wdg::CModuleWatchdog::Instance()->Feeddog(key);
  }
  return -1;
}

int WatchDog_Enable(int enable) {
  if (wdg::CModuleWatchdog::Instance()) {
    return wdg::CModuleWatchdog::Instance()->Enable(enable);
  }
  return -1;
}
