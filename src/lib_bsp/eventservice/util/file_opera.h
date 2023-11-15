#ifndef __FILE_DIR_H__
#define __FILE_DIR_H__
#include <string>
#include <vector>

#include "eventservice/base/basictypes.h"
#include "eventservice/mem/membuffer.h"

namespace chml {

/** 定义输入输出模式 */
typedef enum {
  IO_MODE_RD_ONLY,        /**< 只可读(r) */
  IO_MODE_WR_ONLY,        /**< 只可写 */
  IO_MODE_RDWR_ONLY,      /**< 只可读写(r+) */
  IO_MODE_APPEND_ONLY,    /**< 只增加 */
  IO_MODE_REWR_ORNEW,     /**< 文件重写,没有则创建(w) */
  IO_MODE_RDWR_ORNEW,     /**< 文件可读写,没有则创建(w+) */
  IO_MODE_APPEND_ORNEW,   /**< 文件可增加,没有则创建(a) */
  IO_MODE_RDAPPEND_ORNEW, /**< 文件可读或可增加,没有则创建(a+) */
  IO_MODE_INVALID = 0xFF,
} IO_MODE_E;

/**
 *  \file   FileUtil.h
 *  \class  File
 *  \brief  文件类
 */
class XFile {
 public:
  XFile();
  virtual ~XFile();

  /* 获取文件大小 */
  static int64 get_content_size(const char *filename);

  /* 读文件内容 */
  static int32 read_all(const char *filename, std::string &content);
  static int32 read_all(const char *filename, chml::MemBuffer::Ptr buf);

  std::string get_name();

  virtual bool open(const char *filename, int32 io_mode);
  bool is_open();
  bool is_end();

  virtual bool close();

  virtual int32 read_data(char *buf, int32 count, int32 usTimeout = -1);
  virtual int32 write_data(const char *buf, int32 count, int32 usTimeout = -1);

#ifndef WIN32
  void *map_memory();
  int32 unmap_memory();
#endif  // WIN32

  int32 get_size();
  int32 get_pos();
  int32 set_pos(int32 pos);
  int32 read_line(std::string &line_string);

 private:
  FILE *file_;
  void *map_addr_;
  std::string filename_;
};

}
#endif