#ifndef __VTY_COMMON_H__
#define __VTY_COMMON_H__

#include <vector>

#include "eventservice/base/basictypes.h"
#include "eventservice/base/sigslot.h"
#include "eventservice/base/criticalsection.h"
#include "eventservice/log/log_client.h"
#include "boost/boost/boost_settings.hpp"

#include "eventservice/vty/vty_client.h"
#include "eventservice/vty/libtelnet.h"


namespace vty {

#define VTY_WARNNING_PREFIX				"ml_vty warning:"
#define VTY_CMD_PREIFX					"$"

#define VTY_CMD_BRIEFSTRING_LEN         8			//输出简短信息时的最大长度

//server线程
#define MSG_ON_PROCESSOR_DONE           1001		//proc线程任务执行完成
//proc线程
#define MSG_ON_PROC_CMD                 1002		//新命令执行
#define MSG_ON_PROCESSOR_DISCONNECT     1003		//vty退出 

template <typename Handler>
struct CMD_S {
 public:

  CMD_S(const std::string &cmd,
        Handler hander,
        const void *user_data,
        const std::string &help_info):
    cmd_(cmd), hander_(hander), user_data_(user_data), help_info_(help_info) {
  }
  CMD_S() {}

  std::string ToHelpString() {
    std::string ret = cmd_;
    while (ret.size() < VTY_CMD_MAX_LEN) {
      ret += ' ';
    }
    return ret + " : " + help_info_;
  }
  std::string cmd_;
  Handler hander_;
  const void *user_data_;
  std::string help_info_;
};

template <typename Handler>
class VtyCmdList:
  public boost::noncopyable,
  public sigslot::has_slots<>,
  public boost::enable_shared_from_this<VtyCmdList<Handler> > {
 public:
  typedef typename boost::shared_ptr<VtyCmdList<Handler> > Ptr;
  typedef typename std::list<CMD_S<Handler> >::iterator cmd_list_iter;
  VtyCmdList() {}
  ~VtyCmdList() {}

 public:
  //return: 成功:0, 出错:-1
  int AddCmd(const std::string &cmd,
             Handler hander,
             const void * user_data,
             const std::string &help_info = "no details");

  //return: 成功:0, 不存在该命令:1, 出错:-1
  int RemoveCmd(const std::string &cmd);

  //return: 成功:0， 失败 -1. 对应的CMD_S 写入cmd_s
  int GetCmdStruct(const std::string &cmd, CMD_S<Handler> &cmd_s);

  //return: 所有已经注册命令的格式化字符串
  std::string CmdsToString();

  //在已注册命令中模糊匹配，返回所有找到的命令名称
  std::list<std::string> FindCmd(std::string cmd);

  //查询某命令的信息
  int GetCmdInfo(std::string cmd, std::string &info);

 private:
  bool IsCmdCharLegal(const char &ch);
  std::list<CMD_S<Handler> > list_;
  chml::CriticalSection     crit_;
};


class TelnetEntity :
  public boost::noncopyable,
  public sigslot::has_slots<>,
  public boost::enable_shared_from_this<TelnetEntity> {
 public:
  typedef boost::shared_ptr<TelnetEntity> Ptr;
  TelnetEntity() {
    closed_ = false;
  }
  ~TelnetEntity() {
    if (telnet_ != NULL) {
      telnet_free(telnet_);
    }
    if (telopts_ != NULL) {
      free(telopts_);
    }
  }
  bool Init(std::vector<telnet_telopt_t> telopts,
            telnet_event_handler_t handler,
            unsigned char flags, void *user_data);
  // 接收数据
  int Receive(const char *buffer, size_t size);
  // 打印数据
  int Print(const char *fmt, ...);
  int VPrint(const char *fmt, va_list va);
  void Close();
  bool IsClose();

 public:
 private:
  bool closed_;
  telnet_telopt_t *telopts_;
  telnet_t *telnet_;
  chml::CriticalSection     crit_;
};
/*---------------------------VtyCmdList--------------------------------------*/
template <typename Handler>
int VtyCmdList<Handler>::AddCmd(const std::string &cmd,
                                Handler hander,
                                const void *user_data,
                                const std::string &help_info) {
  ASSERT_RETURN_FAILURE(cmd.size() > VTY_CMD_MAX_REGIST, -1);
  ASSERT_RETURN_FAILURE(hander == NULL, -1);
  ASSERT_RETURN_FAILURE(help_info.size() > VTY_HELPINFO_MAX_LEN, -1);
  for (size_t i = 0; i < cmd.size(); i++) {
    ASSERT_RETURN_FAILURE(!IsCmdCharLegal(cmd[i]), -1);
  }
  {
    chml::CritScope crit(&crit_);
    RemoveCmd(cmd);
    list_.push_back(CMD_S<Handler>(cmd, hander, user_data, help_info));
  }
  return 0;
}

template <typename Handler>
int VtyCmdList<Handler>::RemoveCmd(const std::string &cmd) {
  chml::CritScope crit(&crit_);
  for (cmd_list_iter it = list_.begin(); it != list_.end(); it++) {
    if (it->cmd_ == cmd) {
      list_.erase(it);
      return 0;
    }
  }
  return 1;
}

template <typename Handler>
int VtyCmdList<Handler>::GetCmdStruct(const std::string &cmd,
                                      CMD_S<Handler> &cmd_s) {
  chml::CritScope crit(&crit_);
  for (cmd_list_iter it = list_.begin(); it != list_.end(); it++) {
    if (it->cmd_ == cmd) {
      cmd_s = *it;
      return 0;
    }
  }
  return -1;
}

template <typename Handler>
std::string VtyCmdList<Handler>::CmdsToString() {
  std::string res;
  chml::CritScope crit(&crit_);
  for (cmd_list_iter it = list_.begin(); it != list_.end(); it++) {
    res += it->ToHelpString() + "\n";
  }
  return res;
}

template <typename Handler>
bool VtyCmdList<Handler>::IsCmdCharLegal(const char &ch) {
  if (ch >= 'a' && ch <= 'z') {
    return true;
  }
  if (ch >= 'A' && ch <= 'Z') {
    return true;
  }
  if (ch >= '0' && ch <= '9') {
    return true;
  }
  if (ch == '_') {
    return true;
  }
  return false;
}

template <typename Handler>
std::list<std::string> VtyCmdList<Handler>::FindCmd(std::string cmd) {
  std::list<std::string> res;
  chml::CritScope crit(&crit_);
  for (cmd_list_iter it = list_.begin(); it != list_.end(); it++) {
    std::string str = it->cmd_;
    if (str.find(cmd) != std::string::npos) {
      res.push_back(str);
    }
  }
  return res;
}

template <typename Handler>
int VtyCmdList<Handler>::GetCmdInfo(std::string cmd, std::string &info) {
  chml::CritScope crit(&crit_);
  for (cmd_list_iter it = list_.begin(); it != list_.end(); it++) {
    std::string str = it->cmd_;
    if (str == cmd) {
      info = it->info_;
      return 0;
    }
  }
  return -1;
}

}

#endif  // __VTY_COMMON_H__
