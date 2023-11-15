#ifndef APP_APP_APPINTERFACE_H_
#define APP_APP_APPINTERFACE_H_

#include "eventservice/base/basictypes.h"
#include "eventservice/net/eventservice.h"

namespace app {

class AppInterface {
 public:
  typedef boost::shared_ptr<AppInterface> Ptr;

 public:
  explicit AppInterface(const std::string app_name, size_t thread_stack_size = 0);
  // 各应用准备、初始化数据库等自身环境，应用之间不要相互依赖初始化
  virtual bool PreInit(chml::EventService::Ptr event_service) = 0;
  // 应用之间可以相互之间初始化，在这个阶段各个应用自身已经初始化成功
  virtual bool InitApp(chml::EventService::Ptr event_service) = 0;
  // 正式运行
  virtual bool RunAPP(chml::EventService::Ptr event_service) = 0;
  // This function will be called by other thread
  virtual void OnExitApp(chml::EventService::Ptr event_service) = 0;

  const std::string app_name() const {
    return app_name_;
  }
  const size_t thread_stack_size() {
    return thread_stack_size_;
  }

 protected:
  std::string app_name_;
  size_t thread_stack_size_;
};

}  // namespace app

#endif  // APP_APP_APPINTERFACE_H_
