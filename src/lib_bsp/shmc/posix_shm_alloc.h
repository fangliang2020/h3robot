#ifndef SHMC_POSIX_SHM_ALLOC_H_
#define SHMC_POSIX_SHM_ALLOC_H_

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include "shmc/shm_alloc.h"

namespace shmc {

namespace impl {

class POSIXShmAlloc : public ShmAlloc {
 public:
  virtual ~POSIXShmAlloc() {}

  void* Attach(const std::string& key, size_t size, int flags,
                       size_t* mapped_size) override {
    int oflag = 0;
    if (flags & kCreate)
      oflag |= O_CREAT;
    if (flags & kCreateExcl)
      oflag |= O_EXCL;
    if (flags & kReadOnly)
      oflag |= O_RDONLY;
    else
      oflag |= O_RDWR;
    int fd = shm_open(key.c_str(), oflag, 0777);
    if (fd < 0) {
      set_last_errno(conv_errno());
      return nullptr;
    }
    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
      set_last_errno(conv_errno());
      close(fd);
      return nullptr;
    }
    size_t shm_size = file_stat.st_size;
    if (size > shm_size) {
      // kCreate means we can create new shm or enlarge old one
      if (flags & kCreate) {
        if (ftruncate(fd, size) < 0) {
          set_last_errno(conv_errno());
          close(fd);
          return nullptr;
        }
        shm_size = size;
      } else {
        set_last_errno(kErrBiggerSize);
        close(fd);
        return nullptr;
      }
    }
    int mmap_prot = PROT_READ;
    if (!(flags & kReadOnly))
      mmap_prot |= PROT_WRITE;
    void* addr = mmap(nullptr, shm_size, mmap_prot, MAP_SHARED, fd, 0);
    if (addr == reinterpret_cast<void*>(-1)) {
      set_last_errno(conv_errno());
      close(fd);
      return nullptr;
    }
    close(fd);
    if (mapped_size) {
      *mapped_size = shm_size;
    }
    return addr;
  }

  bool Detach(void* addr, size_t size) override {
    if (munmap(addr, size) < 0) {
      set_last_errno(conv_errno());
      return false;
    }
    return true;
  }

  bool Unlink(const std::string& key) override {
    if (shm_unlink(key.c_str()) < 0) {
      set_last_errno(conv_errno());
      return false;
    }
    return true;
  }
  size_t AlignSize() override {
    return 1;
  }

 private:
  static ShmAllocErrno conv_errno() {
    switch (errno) {
    case 0:
      return kErrOK;
    case ENOENT:
      return kErrNotExist;
    case EEXIST:
      return kErrAlreadyExist;
    case EINVAL:
      return kErrInvalid;
    case EACCES:
    case EPERM:
      return kErrPermission;
    default:
      return kErrDefault;
    }
  }
};

}  // namespace impl

using POSIXS = impl::POSIXShmAlloc;

}  // namespace shmc

#endif  // SHMC_POSIX_SHM_ALLOC_H_
