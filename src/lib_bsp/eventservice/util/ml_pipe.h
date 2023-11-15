#pragma once

#include <string>
#include <fstream>

namespace chml {
class MLPipe {
public:
  MLPipe() : fd_(-1) {}
  MLPipe(const std::string &full_path);
  MLPipe(const char *path, const char *name);
  int32_t SetBlock(bool enbale);

  int32_t Open(const std::string &full_path);
  int32_t Open(const char *path, const char *name);
  void Close();

  int32_t Read(char *s, size_t n);
  int32_t Write(const char *s, size_t n);

  bool IsOpen();

  ~MLPipe();
private:
  // std::fstream pipe_;
  int32_t fd_;
};
}

