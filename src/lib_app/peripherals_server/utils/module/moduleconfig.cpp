#include "moduleconfig.h"

ModuleConfig::ModuleConfig() {
}

ModuleConfig::~ModuleConfig() {
}

ModuleConfig* ModuleConfig::Intance() {
  static ModuleConfig* pthiz = NULL;
  if (NULL == pthiz) {
  pthiz = new ModuleConfig();
  }
  return pthiz;
}

int ModuleConfig::SetConfig(const std::string& key, chml::MessageData::Ptr value) {
  chml::CritScope lock(&config_mutex_);
  std::map<std::string, chml::MessageData::Ptr>::iterator iter = config_map_.find(key);
  if (iter != config_map_.end()) { 
    iter->second = value;
  } else {
  config_map_.insert(std::make_pair(key, value));
  }
  return 0;
}

chml::MessageData::Ptr ModuleConfig::GetConfig(const std::string& key) {
  chml::CritScope lock(&config_mutex_);
  std::map<std::string, chml::MessageData::Ptr>::const_iterator iter = config_map_.find(key);
  return iter == config_map_.end() ? chml::MessageData::Ptr() : iter->second;
}