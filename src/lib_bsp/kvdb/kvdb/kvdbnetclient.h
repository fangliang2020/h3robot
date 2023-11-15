#ifndef __FILECHACHE_KVDB_NETCLIENT_H__
#define __FILECHACHE_KVDB_NETCLIENT_H__

#include "eventservice/net/eventservice.h"
#include "eventservice/log/log_client.h"
#include "eventservice/base/basictypes.h"
#include "kvdboperatefile.h"
#include "kvdbclient.h"

namespace cache {

enum KvdbNetType {
  KVDB_NET_TYPE_SETKEY = 1,
  KVDB_NET_TYPE_GETKEY = 2,
  KVDB_NET_TYPE_DELETEKEY = 3,
  KVDB_NET_TYPE_SETPROPERTY = 4,
  KVDB_NET_TYPE_GETPROPERTY = 5,
  KVDB_NET_TYPE_BACKUPDATABASE = 6,
  KVDB_NET_TYPE_RESTOREDATABASE = 7,
  KVDB_NET_TYPE_CLEAR = 8,
  KVDB_NET_TYPE_INITKVDBNAME = 10,
  KVDB_NET_TYPE_ERROR = 404
};

#define CACHE_PATH_LENGTH_MAX 256

class KvdbNetClientImpl
  : public KvdbClient
  , public boost::noncopyable
  , public boost::enable_shared_from_this<KvdbNetClientImpl>
  , public sigslot::has_slots<> {
 public:
  typedef boost::shared_ptr<KvdbNetClientImpl> Ptr;

 public:
  KvdbNetClientImpl(){
    kvdb_file_ = NULL;
  }

  ~KvdbNetClientImpl() {
    DLOG_INFO(MOD_EB, "Close KvdbNetClient");
    chml::CritScope cr(&crit_);
    if (kvdb_file_ != NULL) {
      delete kvdb_file_;
      kvdb_file_ = NULL;
    }
  }

  //初始化netclient CreateKvdbNetClient内部调用
  bool Init();

  std::string GetKvdbName();
  void SetKvdbName(const std::string &name);

 private:
  std::string GetRealKey(const char *key);

  // local
  bool isKvdbFileValid();

 public:
  // kvdb client interface
  bool SetKey(const char *key, uint8 key_size, const char *value, uint32 value_size);
  bool SetKey(const std::string key, const char *value, uint32 value_size);
  bool GetKey(const char *key, uint8 key_size, std::string *result);
  bool GetKey(const std::string key, std::string *result);
  bool GetKey(const std::string key, void *buffer, std::size_t buffer_size);
  bool GetKey(const char *key, uint8 key_size, void *buffer, std::size_t buffer_size);
  bool DeleteKey(const char *key, uint8 key_size);
  // 设置kvdb的属性值property, 可取值为：KVDB_SAFE_MODE
  // KVDB_SAFE_MODE：以安全模式运行，读取会校验文件正确性，主文件错误会读取校验文件，
  // 若校验文件仍错误则读取失败。
  // return: 设置结果
  bool SetProperty(int property);
  // 获取kvdb的属性值property, 有效的值为：KVDB_SAFE_MODE
  // KVDB_SAFE_MODE：以安全模式运行，读取会校验文件正确性，主文件错误会读取校验文件，
  // 若校验文件仍错误则读取失败。
  // return: 获取结果
  bool GetProperty(int &property);

  //备份当前kvdb数据
  bool BackupDatabase();

  //恢复到此前的kvdb备份版本。若此前没有备份则会清空当前kvdb。
  bool RestoreDatabase();

  //清空kvdb。包括驱动创建的/usr_back文件夹对应的kvdb备份。
  // clear_type：1 主目录。 2 bak目录。3 主目录+bak目录。
  bool Clear(uint8 clear_type = KVDB_CLEAR_TYPE_MAIN | KVDB_CLEAR_TYPE_BAK);

 private:
  chml::CriticalSection     crit_;
  std::string               name_;
  KvdbOperateFile*          kvdb_file_;
};

}  // namespace cache

#endif  // __FILECHACHE_KVDB_NETCLIENT_H__
