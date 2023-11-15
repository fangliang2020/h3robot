#include "kvdbnetclient.h"
#ifdef KVDB_PAUSE
#define KVDB_PAUSE_FILE "/tmp/kvdb_pause"
#endif

namespace cache {

cache::KvdbClient::Ptr KvdbClient::CreateKvdbClient(const std::string& name) {
  KvdbNetClientImpl::Ptr client(new KvdbNetClientImpl());
  if (client.get()) {
    client->SetKvdbName(name);
    if (!client->Init()) {
      DLOG_ERROR(MOD_EB, "CreateKvdbClient failed");
      return cache::KvdbClient::Ptr();
    }
    DLOG_DEBUG(MOD_EB, "create %s client success.", name.c_str());
    return client;
  }
  DLOG_ERROR(MOD_EB, "create net client failed.");
  return cache::KvdbClient::Ptr();
}
#ifdef KVDB_PAUSE
void KvdbClient::Pause() {
  DLOG_ERROR(MOD_EB, "kvdb Pause");
  FILE *fp = fopen(KVDB_PAUSE_FILE, "w+");
  if (fp == NULL) {
    DLOG_ERROR(MOD_EB, "kvdb Pause failed .can not create file %s\n", strerror(errno));
    return ;
  }
  fclose(fp);
}

void KvdbClient::Resume() {
  DLOG_ERROR(MOD_EB, "kvdb Resume.");
  if(remove(KVDB_PAUSE_FILE) == -1){
    DLOG_ERROR(MOD_EB, "kvdb Resume failed .can not remove file %s\n", strerror(errno));
  }
}
#endif
cache::KvdbClient::Ptr KvdbClient::CreateKvdbNetClient(const std::string& name, chml::SocketAddress addr) {
  return CreateKvdbClient(name);
}

bool KvdbNetClientImpl::Init() {
  DLOG_INFO(MOD_EB, "Kvdb net client init request");
  // 是单进程，直接调接口
  kvdb_file_ = new KvdbOperateFile(NULL);
  if (NULL == kvdb_file_) {
    DLOG_INFO(MOD_EB, "kvdb_file_ init failed");
    return false;
  }
  char buff[KVDB_FILE_PATH_MAXLEN] = {0};
  kvdb_file_->GetWholePath(buff, KVDB_FILE_PATH_MAXLEN, KVDB_PARENT_FOLD, GetKvdbName().c_str(), false);
  kvdb_file_->FoldCreate(buff);
  return true;
}

void KvdbNetClientImpl::SetKvdbName(const std::string &name) {
  name_ = name;
}

std::string KvdbNetClientImpl::GetKvdbName() {
  return name_;
}

std::string KvdbNetClientImpl::GetRealKey(const char *key) {
  std::string tmp;
  tmp = GetKvdbName() + "/" + key;
  return tmp;
}

bool KvdbNetClientImpl::isKvdbFileValid() {
  if (kvdb_file_ == nullptr) {
    return false;
  }
  return true;
}

bool KvdbNetClientImpl::SetKey(const char *key, uint8 key_size,
                               const char *value, uint32 value_size) {
  if (!isKvdbFileValid()) {
    DLOG_WARNING(MOD_EB, "KvdbOperateFile not inited!");
    return false;
  }
#ifdef KVDB_PAUSE
  if(access(KVDB_PAUSE_FILE, R_OK) == 0){
    DLOG_INFO(MOD_EB, "kvdb paused. key loss %.*s!", key_size, key);
    return true;
  }
#endif  
  KvdbError rslt = kvdb_file_->WriteKey(GetRealKey(key), value, value_size);
  
  if (rslt == KVDB_ERR_OK) {
    return true;
  }
  DLOG_WARNING(MOD_EB, "%s WriteKey faild %d!", key, rslt);
  return false;
}

bool KvdbNetClientImpl::SetKey(const std::string key, const char *value, uint32 value_size) {
  return SetKey(key.c_str(), key.size(), value, value_size);
}

bool KvdbNetClientImpl::GetKey(const char *key, uint8 key_size,
                               std::string *result) {
  if (!isKvdbFileValid()) {
    DLOG_WARNING(MOD_EB, "KvdbOperateFile not inited!");
    return false;
  }
  KvdbError rslt = kvdb_file_->ReadKey(GetRealKey(key), *result);
  if (rslt == KVDB_ERR_OK) {
    return true;
  }
  DLOG_WARNING(MOD_EB, "%s ReadKey faild %d!", key, rslt);
  return false;
}

bool KvdbNetClientImpl::GetKey(const std::string key, std::string *result) {
  // 防止多线程并发错误
  chml::CritScope crit(&crit_);
  return GetKey(key.c_str(), key.size(), result);
}

bool KvdbNetClientImpl::GetKey(const std::string key, void *buffer, std::size_t buffer_size) {
  // 防止多线程并发错误
  chml::CritScope crit(&crit_);
  return GetKey(key.c_str(), key.size(), buffer, buffer_size);
}

bool KvdbNetClientImpl::GetKey(const char *key, uint8 key_size, void *buffer, std::size_t buffer_size) {
  // 防止多线程并发错误
  chml::CritScope crit(&crit_);
  std::string value;
  int res = GetKey(key, key_size, &value);
  if (!res || value.size() > buffer_size) {
    return false;
  } else {
    memcpy(buffer, value.c_str(), value.size());
    return true;
  }
}

bool KvdbNetClientImpl::DeleteKey(const char *key, uint8 key_size) {
  if (!isKvdbFileValid()) {
    DLOG_WARNING(MOD_EB, "KvdbOperateFile not inited!");
    return false;
  }

  KvdbError rslt = kvdb_file_->Remove(GetRealKey(key));
  if (rslt == KVDB_ERR_OK) {
    return true;
  }
  DLOG_WARNING(MOD_EB, "%s Remove faild %d!", key, rslt);
  return false;
}

bool KvdbNetClientImpl::SetProperty(int property) {
  if (!isKvdbFileValid()) {
    DLOG_WARNING(MOD_EB, "KvdbOperateFile not inited!");
    return false;
  }

  KvdbError rslt = kvdb_file_->SetProperty(property);
  if (rslt == KVDB_ERR_OK) {
    return true;
  }
  DLOG_WARNING(MOD_EB, "SetProperty faild %d!", rslt);
  return false;
}

bool KvdbNetClientImpl::GetProperty(int &property) {
  if (!isKvdbFileValid()) {
    DLOG_WARNING(MOD_EB, "KvdbOperateFile not inited!");
    return false;
  }

  KvdbError rslt = kvdb_file_->GetProperty(property);
  if (rslt == KVDB_ERR_OK) {
    return true;
  }
  DLOG_WARNING(MOD_EB, "SetProperty faild %d!", rslt);
  return false;
}

bool KvdbNetClientImpl::BackupDatabase() {
  if (!isKvdbFileValid()) {
    DLOG_WARNING(MOD_EB, "KvdbOperateFile not inited!");
    return false;
  }

  KvdbError rslt;
  char bak_foldpath[CACHE_PATH_LENGTH_MAX];
  kvdb_file_->GetWholePath(bak_foldpath, CACHE_PATH_LENGTH_MAX, KVDB_PARENT_FOLD, GetKvdbName().c_str(), true);
  rslt = kvdb_file_->Backup(bak_foldpath, GetKvdbName().c_str());
  if (rslt == KVDB_ERR_OK) {
    return true;
  }
  DLOG_WARNING(MOD_EB, "Backup faild %d!", rslt);
  return false;
}

bool KvdbNetClientImpl::RestoreDatabase() {
  if (!isKvdbFileValid()) {
    DLOG_WARNING(MOD_EB, "KvdbOperateFile not inited!");
    return false;
  }

  KvdbError rslt;
  char foldpath[CACHE_PATH_LENGTH_MAX];
  kvdb_file_->GetWholePath(foldpath, CACHE_PATH_LENGTH_MAX, KVDB_PARENT_FOLD, GetKvdbName().c_str(), true);
  rslt = kvdb_file_->Restore(foldpath, GetKvdbName().c_str());

  // backup main folder
  kvdb_file_->GetWholePath(foldpath, CACHE_PATH_LENGTH_MAX, KVDB_BACKUP_PARENT_FOLD, GetKvdbName().c_str(), false);
  rslt = kvdb_file_->Clear(foldpath);
  if (rslt == KVDB_ERR_OK) {
    return true;
  }
  DLOG_WARNING(MOD_EB, "Clear folder %s failed.", foldpath);
  return false;
}

bool KvdbNetClientImpl::Clear(uint8 clear_type) {
  if (!isKvdbFileValid()) {
    DLOG_WARNING(MOD_EB, "KvdbOperateFile not inited!");
    return false;
  }

  KvdbError rslt;
  char foldpath[CACHE_PATH_LENGTH_MAX];
  if (clear_type & KVDB_CLEAR_TYPE_MAIN) {
    // main folder
    kvdb_file_->GetWholePath(foldpath, CACHE_PATH_LENGTH_MAX, KVDB_PARENT_FOLD, GetKvdbName().c_str(), false);
    rslt = kvdb_file_->Clear(foldpath);
    if (rslt != KVDB_ERR_OK) {
      DLOG_WARNING(MOD_EB, "Clear folder %s failed.", foldpath);
    }

    // backup main folder
    kvdb_file_->GetWholePath(foldpath, CACHE_PATH_LENGTH_MAX, KVDB_BACKUP_PARENT_FOLD, GetKvdbName().c_str(), false);
    rslt = kvdb_file_->Clear(foldpath);
    if (rslt != KVDB_ERR_OK) {
      DLOG_WARNING(MOD_EB, "Clear folder %s failed.", foldpath);
    }
  }

  if (clear_type & KVDB_CLEAR_TYPE_BAK) {
    // backup bak folder
    kvdb_file_->GetWholePath(foldpath, CACHE_PATH_LENGTH_MAX, KVDB_BACKUP_PARENT_FOLD, GetKvdbName().c_str(), true);
    rslt = kvdb_file_->Clear(foldpath);
    if (rslt != KVDB_ERR_OK) {
      DLOG_WARNING(MOD_EB, "Clear folder %s failed.", foldpath);
    }

    // bak folder
    bool bak_flag = true;
    if(strcmp(GetKvdbName().c_str(), KVDB_NAME_SKVDB) == 0) {
      bak_flag = false;
    }
    kvdb_file_->GetWholePath(foldpath, CACHE_PATH_LENGTH_MAX, KVDB_PARENT_FOLD, GetKvdbName().c_str(), bak_flag);
    rslt = kvdb_file_->Clear(foldpath);
    if (rslt != KVDB_ERR_OK) {
      DLOG_WARNING(MOD_EB, "Clear folder %s failed.", foldpath);
      return false;
    }
  }
  return true;
}

} // namespace cache
