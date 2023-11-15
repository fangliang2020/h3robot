// 文件系统操作类

#ifndef _OS_FILE_UTIL_H_
#define _OS_FILE_UTIL_H_

#include "eventservice/base/basictypes.h"

#include <string>
#include <vector>

namespace chml {

///////////////////////////////////////////////////////////////////////////////
// class XFileFinder
///////////////////////////////////////////////////////////////////////////////
class XFileFinder {
 public:
  enum {
    TYPE_NONE = 0x00000000,     // 没有
    TYPE_DIR = 0x00000001,      // 目录
    TYPE_REGULAR = 0x00000002,  // 普通文件
    TYPE_LINK = 0x00000004,     // 符号连接
    TYPE_CHAR = 0x00000008,     // 字符特殊文件
    TYPE_BLOCK = 0x00000010,    // 块特殊文件
    TYPE_FIFO = 0x00000020,     // 管道
    TYPE_SOCKET = 0x00000040,   // 套接字
    TYPE_OTHER = 0x00000080,    // 其他类型
    TYPE_ALL = 0x000000FF,      // 所有文件类型

    ATTR_HIDE = 0x00000100,    // 隐藏
    ATTR_SYSTEM = 0x00000200,  // 系统文件
    ATTR_ALL = 0x00000300      // 所有属性
  };

 public:
  XFileFinder();
  ~XFileFinder();

  bool open(const std::string &path, uint32_t mask = TYPE_ALL | ATTR_ALL);
  int  next(std::string &name);
  void close();

 private:
  void *m_finder;
  uint32_t m_mask;
};

///////////////////////////////////////////////////////////////////////////////
// class XFileUtil
///////////////////////////////////////////////////////////////////////////////
class XFileUtil {
 public:
  static char PATH_SEPARATOR;

 public:
  static bool create_file(const std::string &filepath);

  static bool make_dir(const std::string &dir_path);
  static bool make_dir_p(const std::string &dir_path);
  static bool remove_dir(const std::string &dir_path);
  static bool remove_dir_r(const std::string &dir_path);
  static bool remove_file(const std::string &file_path);
  static bool make_link(const std::string &strTargetPath, const std::string &strNewPath, bool overwrite = false);
  static bool make_link_p(const std::string &strTargetPath, const std::string &strNewPath);

  // check type of file or dir
  static bool is_link(const std::string &strpath, /*out*/ std::string &strTargetPath);
  static bool is_dir(const std::string &strPath);
  static bool is_regular(const std::string &strPath);
  static bool is_exist(const std::string &strPath);

  static bool parse_path(const std::string &path, std::string &dir_path,
                         std::string &filename_perfix,
                         std::string &filename_postfix);

  static bool set_work_dir(const std::string &dir_path);
  static std::string get_work_dir();
  static std::string get_module_path();
  static std::string get_system_dir();

  static uint32_t get_page_size();
  static uint32_t get_name_max(const char *dir_path = NULL);
  static uint32_t get_path_max(const char *dir_path = NULL);

  // 获取目录下所有文件
  static void get_path_file(const std::string &strpath, std::vector<std::string>& vecfile, bool deep = false);
};

}  // namespace chml

#endif  //_XCORE_FILE_UTIL_H_
