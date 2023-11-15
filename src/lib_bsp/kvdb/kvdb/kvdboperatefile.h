ifndef SRC_LIBMLTINYDB_KvdbOperateFile_H_
#define SRC_LIBMLTINYDB_KvdbOperateFile_H_

#include <string>
#include <stdio.h>
#include <time.h>
#include "eventservice/base/basictypes.h"
#include "eventservice/base/sigslot.h"
#include "eventservice/base/criticalsection.h"
#include "eventservice/log/log_client.h"
#include "boost/boost/boost_settings.hpp"
#include "kvdbparamcheck.h"

namespace cache {

#define KVDB_FILE_PATH_MAXLEN    128   ///< 文件全路径名称长度
#define KVDB_KEY_MAXLEN          64    ///< 关键字字符数限制

/**@name    错误类型定义
* @{
*/
enum KvdbError {
  KVDB_ERR_OK = 0,              ///< 正确
  KVDB_ERR_OPEN_FILE = 1,       ///< 打开文件错误
  KVDB_ERR_WRITE_FILE = 2,      ///< 写文件错误
  KVDB_ERR_READ_FILE = 3,       ///< 读文件错误
  KVDB_ERR_PARSE_FILE = 4,      ///< 解析文件错误
  KVDB_ERR_INVALID_PARAM = 5,   ///< 无效参数错误
  KVDB_ERR_SEEK_NULL = 6,       ///< 查找结果为空
  KVDB_ERR_FULL = 7,            ///< 数据库满错误
  KVDB_ERR_DELETE_FILE = 8,     ///< 文件删除错误
  KVDB_ERR_OPEN_FOLD = 9,       ///< 打开文件夹错误
  KVDB_ERR_FILE_CHECK = 10,     ///< 文件校验错误
  KVDB_ERR_FILE_DONTEXIST = 11, ///< 文件不存在
  KVDB_ERR_UNKNOWN = 99         ///< 未知错误
};
/**@} */

/**@name    状态标识位定义
* @{
*/
#define KVDB_FILE_HEAD_FLAG      0x565a
/**@} */


class KvdbOperateFile {
 public:
  /**
  * @brief 构造函数
  * 根据文件名创建skvdb和普通kvdb，业务层实现不同操作规则
  * 缓存序列需要根据长度由小到达排列
  * LITEOS下默认不开设缓存，变量cache，cache_cnt失效
  * @param[in] foldpath    文件夹路径
  * @param[in] cache       缓存类型定义序列
  * @param[in] cache_cnt   缓存类型定义个数
  */
  KvdbOperateFile(const char *foldpath, int property = 1);
  /**
  * @brief 析构函数
  */
  ~KvdbOperateFile();
  /**
  * @brief 修改或增加一条记录
  * 如果已保存相同关键字记录就修改，否则新增
  * @param[in] key    关键字
  * @param[in] value    值
  * @param[in] length   值长度
  *
  * @return 返回操作结果
  */
  KvdbError WriteKey(const std::string &key, const char *value, int length);
  /**
  * @brief 修改或增加一条记录
  * 如果已保存相同关键字记录就修改，否则新增
  * @param[in] key    关键字
  * @param[in] value    值
  *
  * @return 返回操作结果
  */
  KvdbError WriteKey(const std::string &key, const std::string &value);
  /**
  * @brief 删除记录
  * @param[in] key    关键字
  *
  * @return 返回操作结果
  */
  KvdbError Remove(const std::string &key);
  /**
  * @brief 清空记录
  * @param[in] target_fold    待清空的文件夹目录
  *
  * @return 返回操作结果
  */
  KvdbError Clear(const char *target_fold = NULL);
  /**
  * @brief 查找记录
  * 根据关键字查找记录
  * @param[in] key     待查关键字
  * @param[out] get_value     查询结果
  *
  * @return 返回操作结果
  */
  KvdbError ReadKey(const std::string &key, std::string &get_value);
  /**
  * @brief 备份
  *  将当前文件夹文件备份到备份文件夹
  * @param[in] backup_fold    备份文件夹路径
  *
  * @return 返回操作结果
  */
  KvdbError Backup(const char *backup_fold, const char *name);
  /**
  * @brief 还原
  *  将备份文件夹文件备份到当前文件夹
  * @param[in] backup_fold    备份文件夹路径
  *
  * @return 返回操作结果
  */
  KvdbError Restore(const char *backup_fold, const char *name);

  KvdbError GetProperty(int &property);
  KvdbError SetProperty(int property);

  /**
  * @brief 获取路径
  *  组合完整路径
  * @param[in] maxlen 长度限制  parentfold 前路径  
  *            endpath 后路径   backupflag 备份标记
  *
  * @return foldpath
  */
  void GetWholePath(char *foldpath, int32 maxlen, 
                    const char *parentfold, const char *endpath,
                    bool backupflag);
  /**
  * @brief 创建文件夹
  */
  KvdbError FoldCreate(const char *fold_path);
 private:
  KvdbError ReadFile(const std::string &key, std::string &get_data, bool isbak);
  KvdbError WriteFile(const std::string &key, uint8 *buffer, int length, bool isbak);
  /**
  * @brief 复制文件夹
  */
  KvdbError FoldCopy(const char *target_fold, const char *src_fold);
  /**
  * @brief 复制文件
  */
  KvdbError FileCopy(const char *target_filename, const char *src_filename);

  chml::CriticalSection  crit_; /* 线程互斥锁 */
  char                foldpath_[KVDB_FILE_PATH_MAXLEN + 1];
  int                 property_;
  KvdbParamCheck*     kvdb_check_;
};
}

#endif  // SRC_LIBMLTINYDB_KvdbOperateFile_H_
