#ifndef FILECHACHE_KVDB_KVDBCLIENT_H_
#define FILECHACHE_KVDB_KVDBCLIENT_H_

#include "eventservice/base/basictypes.h"
#include "eventservice/base/socketaddress.h"

#include "boost/boost/boost_settings.hpp"

namespace cache {

// kvdb存储路径
#ifdef WIN32
#define KVDB_PARENT_FOLD "D:\\kvdb"
#define KVDB_BACKUP_PARENT_FOLD "D:\\backup\\kvdb"
#elif defined LITEOS
#define KVDB_PARENT_FOLD "/usr/kvdb"
#define KVDB_BACKUP_PARENT_FOLD "/usr_backup/kvdb"
#elif defined UBUNTU64
#define KVDB_PARENT_FOLD "/tmp/usr/kvdb"
#define KVDB_BACKUP_PARENT_FOLD "/tmp/usr_backup/kvdb"
#else
#define KVDB_PARENT_FOLD "/mnt/usr/kvdb"
#define KVDB_BACKUP_PARENT_FOLD "/mnt/usr_backup/kvdb"
#endif

#define KVDB_NAME_KVDB "kvdb"
#define KVDB_NAME_SKVDB "skvdb"
#define KVDB_NAME_USRKVDB "usr_data_kvdb"

#define KVDB_SUCCEED true
#define KVDB_FAILURE false
#define KVDB_SAFE_MODE 0x01  // KVDB安全模式掩码

#define KVDB_CLEAR_TYPE_MAIN 0x01
#define KVDB_CLEAR_TYPE_BAK  0x02

#ifndef ML_KVDB_SERVER_HOST
#define ML_KVDB_SERVER_HOST "127.0.0.1"
#endif

#ifndef ML_KVDB_SERVER_PORT
#define ML_KVDB_SERVER_PORT 6102
#endif

class KvdbClient {
 protected:
  ~KvdbClient() = default;

 public:
  typedef boost::shared_ptr<KvdbClient> Ptr;

  static KvdbClient::Ptr CreateKvdbClient(const std::string &name);
  static KvdbClient::Ptr CreateKvdbNetClient(const std::string &name,
                                             chml::SocketAddress addr=chml::SocketAddress());
#ifdef KVDB_PAUSE
  static void Pause();
  static void Resume();
#endif
 public:
  virtual bool SetKey(const char *key, uint8 key_size, const char *value,
                      uint32 value_size) = 0;
  virtual bool SetKey(const std::string key, const char *value,
                      uint32 value_size) = 0;

  virtual bool GetKey(const char *key, uint8 key_size, std::string *result) = 0;
  virtual bool GetKey(const std::string key, std::string *result) = 0;
  virtual bool GetKey(const std::string key, void *buffer,
                      std::size_t buffer_size) = 0;
  virtual bool GetKey(const char *key, uint8 key_size, void *buffer,
                      std::size_t buffer_size) = 0;

  virtual bool DeleteKey(const char *key, uint8 key_size) = 0;

  // 设置kvdb的属性值property, 可取值为：KVDB_SAFE_MODE
  // KVDB_SAFE_MODE：以安全模式运行，读取会校验文件正确性，主文件错误会读取校验文件，
  // 若校验文件仍错误则读取失败。
  // return: 设置结果
  virtual bool SetProperty(int property) = 0;
  // 获取kvdb的属性值property, 有效的值为：KVDB_SAFE_MODE
  // KVDB_SAFE_MODE：以安全模式运行，读取会校验文件正确性，主文件错误会读取校验文件，
  // 若校验文件仍错误则读取失败。
  // return: 获取结果
  virtual bool GetProperty(int &protperty) = 0;

  //备份当前kvdb数据
  virtual bool BackupDatabase() = 0;

  //恢复到此前的kvdb备份版本。若此前没有备份则会清空当前kvdb。
  virtual bool RestoreDatabase() = 0;

  //清空kvdb。包括驱动创建的/usr_back文件夹对应的kvdb备份。
  // clear_type：1 主目录。 2 bak目录。3 主目录+bak目录。
  virtual bool Clear(uint8 clear_type = KVDB_CLEAR_TYPE_MAIN | KVDB_CLEAR_TYPE_BAK) = 0;
};

}  // namespace cache

#endif  // FILECHACHE_KVDB_KVDBCLIENT_H_
