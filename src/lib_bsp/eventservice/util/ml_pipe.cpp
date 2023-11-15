#include "ml_pipe.h"
#include <fcntl.h>
#include <libgen.h>
#include <memory.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace chml;

MLPipe::MLPipe(const std::string &full_path) {
  Open(full_path);
}

MLPipe::MLPipe(const char *path, const char *name) {
  Open(path, name);
}

int32_t MLPipe::Open(const std::string &full_path) {
  int32_t ret = 0;
  char *full_path_c = strdup(full_path.c_str());
  char *path_c = dirname(full_path_c);

  if (access(path_c, F_OK) != 0) {
    mkdir(path_c, 0777);
  }

  if (access(full_path.c_str(), F_OK) != 0) {
    ret = mkfifo(full_path.c_str(), 0777);
    if (ret < 0) {
      printf("create fifo failed, %s\n", strerror(errno));
      ret = -1;
      goto out;
    }
  }

  fd_ = open(full_path.c_str(), O_RDWR);
  if (fd_ < 0) {
    printf("open %s failed, err: %s\n", full_path.c_str(), strerror(errno));
    ret = -1;
    goto out;
  }

out:
  free(full_path_c);
  return ret;
}

int32_t MLPipe::Open(const char *path, const char *name) {
  std::string full_path;
  full_path += path;
  full_path += "/";
  full_path += name;
  return Open(full_path);
}

void MLPipe::Close() {
  if (fd_ > 0) {
    close(fd_);
  }
}

int32_t MLPipe::SetBlock(bool enable) {
  if (fd_ < 0) {
    printf("pipe file not open\n");
    return -1;
  }
  int flags = fcntl(fd_, F_GETFL, 0);
  if (enable) {
    flags &= ~O_NONBLOCK;
  } else {
    flags |= O_NONBLOCK;
  }
  if (fcntl(fd_, F_SETFL, flags) < 0) {
    printf("fcntl set failed: %s\n", strerror(errno));
    return -1;
  }

  return 0;
}

bool MLPipe::IsOpen() {
  return fd_ > 0;
}

int32_t MLPipe::Read(char *s, size_t n) {
  return read(fd_, s, n);
}

int32_t MLPipe::Write(const char *s, size_t n) {
  return write(fd_, s, n);
}

MLPipe::~MLPipe() {
  Close();
}
