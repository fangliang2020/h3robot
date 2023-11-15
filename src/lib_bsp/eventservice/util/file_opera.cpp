#include "file_opera.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <algorithm>

#ifndef WIN32
#include <unistd.h>
#include <sys/mman.h>
#include <dirent.h>
#include <libgen.h>
#endif

namespace chml {
/**
 *  \brief  XFile构造函数
 *  \param  none
 *  \note   none
 */
XFile::XFile() :
    file_(nullptr),
    map_addr_(nullptr),
    filename_("") {
}

XFile::~XFile() {
  this->close();
}

/**
 *  \brief  获取内容大小
 *  \param  filePath 文件名或文件夹名
 *  \return 返回文件的大小,以byte为单位
 *  \note   none
 */
#ifndef WIN32
int64 XFile::get_content_size(const char *filename) {
  int fd = ::open(filename, O_RDONLY);
  if (fd == -1){
    printf("open file wrong \n");
    return 0 ;
  }

  off_t off = lseek(fd, 0, SEEK_END);
  ::close(fd);
  return off;
}
#else
int64 XFile::get_content_size(const char * filename) {
  int64 file_len = 0;
  FILE* pf = fopen(filename, "rb");
  if (pf) {
    _fseeki64(pf, 0, SEEK_END);
    file_len = _ftelli64(pf);
    fclose(pf);
  }
  return file_len;
}
#endif

int32 XFile::read_all(const char *filename, std::string &content) {
  FILE* pf = fopen(filename, "rb");
  if (pf) {
    fseek(pf, 0, SEEK_END);
    int file_len = ftell(pf);
    content.resize(file_len);

    fseek(pf, 0, SEEK_SET);
    file_len = fread((char*)&content.c_str()[0], 1, file_len, pf);

    fclose(pf);
  }
  return content.size();
}

int32 XFile::read_all(const char *filename, chml::MemBuffer::Ptr data) {
  if (!data) return -1;
  int64 filesize = get_content_size(filename);
  if(!data->ReadFile(filename, 0, filesize)) {
    return 0;
  }
  return (int32)filesize;
}

/**
 *  \brief  文件是否已经打开
 *  \param  void
 *  \return 文件已打开返回true,否则返回false
 *  \note   none
 */
bool XFile::is_open() {
  if (file_ == NULL) {
    return false;
  }
  return true;
}

/**
 *  \brief  打开文件
 *  \param  filename 文件全名(包含路径,如:/etc/init.d/rcS)
 *  \param  io_mode 打开模式IO_MODE_E
 *  \return 文件打开成功返回true,失败返回false
 *  \note   none
 */
bool XFile::open(const char *filename, int32 io_mode) {
  if (filename == NULL) {
    printf("file name is NULL.\n");
    return false;
  }
  const char *s_mode = "rb";
  switch (io_mode) {
  case IO_MODE_RD_ONLY:
    s_mode = "rb";
    break;
  case IO_MODE_WR_ONLY:
  case IO_MODE_RDWR_ONLY:
    s_mode = "rb+";
    break;
  case IO_MODE_REWR_ORNEW:
    s_mode = "wb";
    break;
  case IO_MODE_RDWR_ORNEW:
    s_mode = "wb+";
    break;
  case IO_MODE_APPEND_ORNEW:
    s_mode = "ab";
    break;
  case IO_MODE_RDAPPEND_ORNEW:
    s_mode = "ab+";
    break;
  default:
    printf("Unsupport mode(%d)\n", io_mode);
    return false;
  }

  if (file_ != NULL) {
    fclose(file_);
    file_ = NULL;
  }

  file_ = fopen(filename, s_mode);
  if (NULL == file_) {
    printf("open file:%s , case: %s.\n", filename, strerror(errno));
    return false;
  }
  filename_ = filename;
  return true;
}

/**
 *  \brief  关闭文件
 *  \param  void
 *  \return 文件关闭成功返回true,失败返回false
 *  \note   none
 */
bool XFile::close() {
  if (file_ != NULL) {
    fclose(file_);
    file_ = NULL;
  }
  return true;
}

/**
 *  \brief  读取数据
 *  \param  buf 数据缓存
 *  \param  count 要读取的长度
 *  \return 成功返回读取的数据长度,失败返回-1
 *  \note   none
 */
int32 XFile::read_data(char *buf, int32 count, int32 usTimeout) {
  if (NULL == file_) {
    printf("XFile not open.\n");
    return -1;
  }
  if (NULL == buf || count <= 0) {
    printf("parameter error:buf=%p,count=%d.\n", buf, count);
    return -1;
  }
  return fread(buf, 1, count, file_);
}

/**
 *  \brief  写入数据
 *  \param  buf 数据缓存
 *  \param  count 要写入的长度
 *  \return 成功返回写入的数据长度,失败返回-1
 *  \note   none
 */
int32 XFile::write_data(const char *buf, int32 count, int32 usTimeout) {
  int32 ret;
  if (NULL == file_) {
    printf("XFile not open.\n");
    return -1;
  }
  if (NULL == buf || count <= 0) {
    printf("parameter error.\n");
    return -1;
  }
  ret = fwrite(buf, 1, count, file_);
  fflush(file_);
  return ret;
}

#ifndef WIN32
/**
 *  \brief  将文件映射到内存
 *  \return 映射成功返回地址,失败返回NULL
 *  \note   在使用mapMemery时,请不要使用readData,writeData这些接口
 */
void *XFile::map_memory() {
  if (map_addr_ == NULL) {
    map_addr_ = mmap(NULL, get_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fileno(file_), 0);
    if (MAP_FAILED == map_addr_) {
      printf("file[%s] map memory !\n", filename_.c_str());
      return NULL;
    }
  }
  return map_addr_;
}

/**
 *  \brief  解除文件内存映射
 *  \return 成功返回0,失败返回-1
 *  \note   在使用mapMemery时,请不要使用readData,writeData这些接口
 */
int32 XFile::unmap_memory() {
  if (map_addr_ != NULL) {
    if (-1 == munmap(map_addr_, get_size())) {
      printf("file[%s] unmap memory !\n", filename_.c_str());
      return -1;
    }
    map_addr_ = NULL;
  }
  return 0;
}
#endif  // WIN32

/**
 *  \brief  读取一行数据
 *  \param  lineStr 该行字符串
 *  \return 成功返回读取的数据长度,失败返回-1
 *  \note   none
 */
int32 XFile::read_line(std::string &lineStr) {
  int32 ret;
  if (NULL == file_) {
    printf("XFile not open.\n");
    return -1;
  }
  lineStr = "";
  while (1) {
    char buf[2] = {0};
    ret = read_data(buf, 1);
    if (-1 == ret) {
      return -1;
    } else if (0 == ret) {
      if (is_end()) {
        return lineStr.size();
      } else {
        return -1;
      }
    } else {
      lineStr += buf;
      if (std::string::npos != lineStr.find("\r\n")) {
        lineStr = lineStr.substr(0, lineStr.size() - 2);
        return lineStr.size();
      } else if (std::string::npos != lineStr.find("\n")) {
        lineStr = lineStr.substr(0, lineStr.size() - 1);
        return lineStr.size();
      }
    }
  }

}

/**
 *  \brief  获取文件大小
 *  \param  void
 *  \return 成功返回文件大小,失败返回-1
 *  \note   none
 */
int32 XFile::get_size() {
  int32 file_size, pos;
  if (NULL == file_) {
    printf("XFile not open.\n");
    return -1;
  }
  pos = ftell(file_);      /* 保存当前读写位置 */
  if (fseek(file_, 0, SEEK_END) < 0) {
    return -1;
  }
  file_size = ftell(file_);
  if (file_size < 0) {
    file_size = -1;
  }
  fseek(file_, pos, SEEK_SET);  /* 恢复当前读写位置 */
  return file_size;
}

/**
 *  \brief  获取文件名称
 *  \param  void
 *  \return 返回当前文件名称
 *  \note   none
 */
std::string XFile::get_name() {
  return filename_;
}

/**
 *  \brief  判断是否已经到文件尾部
 *  \param  void
 *  \return 是返回true,否返回false
 *  \note   none
 */
bool XFile::is_end() {
  int32 pos = get_pos();
  if (pos == -1 || pos != get_size()) {
    return false;
  }
  return true;
}

/**
 *  \brief  获取当前读写位置
 *  \param  void
 *  \return 成功返回当前读写位置,失败返回-1
 *  \note   none
 */
int32 XFile::get_pos() {
  int32 pos = -1;

  if (NULL == file_) {
    printf("XFile not open.\n");
    return -1;
  }

  pos = ftell(file_);
  if (pos < 0) {
    return -1;
  }

  return pos;
}

/**
 *  \brief  设置当前读写位置
 *  \param  void
 *  \return 成功返回0,失败返回-1
 *  \note   none
 */
int32 XFile::set_pos(int32 pos) {
  if (NULL == file_) {
    printf("XFile not open.\n");
    return -1;
  }

  if (0 == fseek(file_, pos, SEEK_SET)) {
    return 0;
  }

  return -1;
}

}