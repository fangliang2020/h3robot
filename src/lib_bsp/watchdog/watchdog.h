#ifndef __WATCH_DOG_C_H__
#define __WATCH_DOG_C_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#if defined WIN32
#define WDG_DEF_MODULE_FILE    "c:\\ml_cfg\\wdg_module.cfg"
#elif defined LITEOS
#define WDG_DEF_MODULE_FILE    "/config_file/wdg_module.cfg"
#elif defined UBUNTU64
#define WDG_DEF_MODULE_FILE    "/tmp/wdg_module.cfg"
#else
#define WDG_DEF_MODULE_FILE    "/tmp/app/exec/wdg_module.cfg"
#endif

#define WDG_DEF_FEEDDOG_TIME   (4)    // 默认喂狗时间(单位:秒)
#define WDG_DEF_TIMEOUT        (20)   // 看门狗默认超时时间(单位:秒)
#define WDG_MIN_TIMEOUT        (4)    // 看门狗最小超时时间(单位:秒)
#define WDG_MAX_TIMEOUT        (61)   // 看门狗最大超时时间(单位:秒)

/* watchdog service */
// 看门狗模块初始化
// return 成功: 0, 失败: -1
int WatchDog_Init(void);

// 看门狗模块检查模块
// return 超时: 1; 未超时 0
int WatchDog_CheckModule(void);


/* subprocess watchdog */
// [子进程使用]看门狗模块初始化
int WatchDog_InitSubProc(void);

// 看门狗模块注册,对于已注册的模块可通过该接口修改超时时间，
// 返回的key值不变。支持多线程并发
// name:模块名称，最大长度32Byte，超长按32Byte截断
// sec_timeout:模块看门狗超时时间，单位:秒，最大值为WDG_MAX_TIMEOUT
// return 成功: > 0，模块Key值; 失败: -1
int WatchDog_Register(const char* name, unsigned int sec_timeout);

int WatchDog_SetTimeout(int key, unsigned int sec_timeout);

// 模块喂狗,支持多线程并发
// key:模块Key值，在WatchDog_Register时返回
// return 成功: 0, 失败: -1
int WatchDog_FeedDog(int key);

// 看门狗动态开关，默认为开启状态。
// 该接口只用于调试使用，正常版本不允许关闭狗
// enable : 1 打开， 0  关闭
// 设置成功:0  设置失败: -1
int WatchDog_Enable(int enable);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  // __WATCH_DOG_C_H__
