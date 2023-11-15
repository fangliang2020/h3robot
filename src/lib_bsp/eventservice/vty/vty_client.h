#ifndef __VTY_CLIETN_H__
#define __VTY_CLIETN_H__

#include "eventservice/base/basictypes.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define VTY_CMD_DEFAULT_INFO    "no details."

#define VTY_CMD_MAX_REGIST          256		//注册命令最多数量
#define VTY_CMD_MAX_LEN             32		//注册命令的最大长度对应VtyCmdHandler的argv[0]长度
#define VTY_HELPINFO_MAX_LEN        64		//注册命令帮助信息的最大长度
#define VTY_CMD_MAX_TOT_LEN         1024	//telnet 客户端输入的单条命令长度最大值
#define VTY_CMD_MAX_PARAM_COUNT     32		//命令参数数量最大值，对应VtyCmdHandler的argc值

#define VTY_CMD_QUEUE_MAX_SIZE      10		//待执行命队列大小
#define VTY_MAX_CONNECTED           3		//telnet 客户端最大连接数

// 命令处理回调函数
// vty_data: 系统数据，调用Vty_Print的时候需要传入此数据
// user_data: 用户注册命令时传入的指针
typedef void(*VtyCmdHandler) (const void *vty_data,
                              const void *user_data,
                              int argc, const char *argv[]);

// Vty模块初始化
// host: 主机字节序的int。0x0 : "0.0.0.0", 0x7F000001 : "127.0.0.1"
// return:成功 0;失败 -1
int  Vty_Init(uint32 host, int port);

// Vty模块销毁
void Vty_Deinit(void);

// 注册Vty命令
// cmd: 命令字符串，有效值: "0-9"、"A～Z"、"a～z"、"_"（下划线) 长度限制 VTY_CMD_MAX_LEN
// cmd_handler: 命令处理回调函数,运行在基础库定义的线程上下文，用户需要考虑并发冲突
// user_data: 用户注册命令可传入数据指针，调用回调函数时会作为参数传入
// info: 帮助用户了解命令用法的信息，长度限制 为VTY_HELPINFO_MAX_LEN
// return:成功 0;失败 -1
int  Vty_RegistCmd(const char *cmd,
                   VtyCmdHandler cmd_handler,
                   const void *user_data,
                   const char *info = VTY_CMD_DEFAULT_INFO);

// 撤销已注册的命令
// 命令字符串，有效值: "0-9"、"A～Z"、"a～z"、"_"（下划线）
// return:成功 0;失败 -1
int  Vty_DeregistCmd(const char *cmd);

// 回调函数中(VtyCmdHandler)打印接口，信息直接输出到客户端shell
// vty_data: VtyCmdHandler 中的参数 vty_data
int Vty_Print(const void *vty_data, const char *format, ...);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  // __VTY_CLIETN_H__
