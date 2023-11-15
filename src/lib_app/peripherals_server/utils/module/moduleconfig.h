#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include "eventservice/event/messagequeue.h"
#include "eventservice/base/criticalsection.h"
// bussiness各模块需要的配置共享
class ModuleConfig {
public:
 ModuleConfig();
 virtual ~ModuleConfig();

 static ModuleConfig* Intance();
public:
  int SetConfig(const std::string& key, chml::MessageData::Ptr value);
  chml::MessageData::Ptr GetConfig(const std::string& key); 
  template<typename T> T GetConfig(const std::string& key, T defaultvalue) {
  chml::CritScope lock(&config_mutex_);
  std::map<std::string, chml::MessageData::Ptr>::const_iterator iter = config_map_.find(key);
  typename chml::TypedMessageData<T>::Ptr pdata;
  return iter == config_map_.end() || !(pdata = boost::static_pointer_cast<chml::TypedMessageData<T>>(iter->second)) ? defaultvalue : pdata->data();
  }
protected:
  std::map<std::string, chml::MessageData::Ptr> config_map_;
  chml::CriticalSection config_mutex_;
};  