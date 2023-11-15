#ifndef SHMC_SVIPC_SHM_ALLOC_H_
#define SHMC_SVIPC_SHM_ALLOC_H_

#ifdef WIN32
#include <windows.h>
#else   // WIN32
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#endif  // WIN32

#include <string>
#include "shmc/shm_alloc.h"
#include "shmc/common_utils.h"

namespace shmc {

namespace impl {

template <bool kEnableHugeTLB>
class SVIPCShmAlloc : public ShmAlloc {
 public:
  virtual ~SVIPCShmAlloc() {}
#ifdef WIN32
  void* Attach(const std::string& key, size_t size, int flags, size_t* mapped_size) override {
    HANDLE shm_hdl = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (LPCSTR)key.c_str());
    if (nullptr == shm_hdl) {
      shm_hdl = ::CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                    0, size, (LPCSTR)key.c_str());
    }
    if (nullptr == shm_hdl) {
      // if got EINVAL when attaching existing shm
      if (errno == EINVAL && !(flags & kCreate)) {
        set_last_errno(kErrBiggerSize);
      } else {
        set_last_errno(conv_errno());
      }
      return nullptr;
    }

    void* addr = ::MapViewOfFile(shm_hdl, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (addr == reinterpret_cast<void*>(-1)) {
      set_last_errno(conv_errno());
      return nullptr;
    }

    if (mapped_size) {
      *mapped_size = size;
    }
    return addr;
  }

  bool Detach(void* addr, size_t size) override {
    if (!::UnmapViewOfFile(addr)) {
      set_last_errno(conv_errno());
      return false;
    }
    return true;
  }

  bool Unlink(const std::string& key) override {
    HANDLE shm_hdl = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (LPCSTR)key.c_str());
    if (NULL == shm_hdl) {
      set_last_errno(conv_errno());
      return false;
    }
    if (!CloseHandle(shm_hdl)) {
      set_last_errno(conv_errno());
      return false;
    }
    return true;
  }
#else  // WIN32
  void* Attach(const std::string& key, size_t size, int flags, size_t* mapped_size) override {
    key_t shm_key;
    if (!str2key(key, &shm_key)) {
      set_last_errno(kErrBadParam);
      return nullptr;
    }
    int shmget_flags = 0777;
    if (flags & kCreate)
      shmget_flags |= IPC_CREAT;
    if (flags & kCreateExcl)
      shmget_flags |= IPC_EXCL;
    if (kEnableHugeTLB) {
      shmget_flags |= SHM_HUGETLB;
      size = shmc::Utils::RoundAlign<kAlignSize_HugeTLB>(size);
    }
    int shm_id = shmget(shm_key, size, shmget_flags);
    if (shm_id < 0) {
      // if got EINVAL when attaching existing shm
      if (errno == EINVAL && !(flags & kCreate)) {
        set_last_errno(kErrBiggerSize);
      } else {
        set_last_errno(conv_errno());
      }
      return nullptr;
    }
    int shmat_flags = 0;
    if (flags & kReadOnly)
      shmat_flags |= SHM_RDONLY;
    void* addr = shmat(shm_id, nullptr, shmat_flags);
    if (addr == reinterpret_cast<void*>(-1)) {
      set_last_errno(conv_errno());
      return nullptr;
    }
    if (mapped_size) {
      struct shmid_ds shmds;
      if (shmctl(shm_id, IPC_STAT, &shmds) < 0) {
        set_last_errno(conv_errno());
        shmdt(addr);
        return nullptr;
      }
      *mapped_size = shmds.shm_segsz;
    }
    return addr;
  }

  bool Detach(void* addr, size_t size) override {
    if (shmdt(addr) < 0) {
      set_last_errno(conv_errno());
      return false;
    }
    return true;
  }

  bool Unlink(const std::string& key) override {
    key_t shm_key;
    if (!str2key(key, &shm_key)) {
      set_last_errno(kErrBadParam);
      return false;
    }
    int shm_id = shmget(shm_key, 0, 0);
    if (shm_id < 0) {
      set_last_errno(conv_errno());
      return false;
    }
    if (shmctl(shm_id, IPC_RMID, nullptr) < 0) {
      set_last_errno(conv_errno());
      return false;
    }
    return true;
  }

  static bool str2key(const std::string& key, key_t* out) {
    // support decimal, octal, hexadecimal format
    errno = 0;
    int64_t shm_key = strtol(key.c_str(), NULL, 0);
    if (errno)
      return false;
    *out = static_cast<key_t>(shm_key);
    return true;
  }
#endif  // WIN32

  size_t AlignSize() override {
    return (kEnableHugeTLB ? kAlignSize_HugeTLB : 1);
  }

 private:
  // 2M-align is a workaround for kernel bug with HugeTLB
  // which has been fixed in new kernels
  // https://bugzilla.kernel.org/show_bug.cgi?id=56881
  static constexpr size_t kAlignSize_HugeTLB = 0x200000UL;

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

class SVIPCMutex {
 public:
  SVIPCMutex() {
#ifdef WIN32
    mutex_handle_ = nullptr;
#else
    mutex_handle_ = 0;
#endif
  }

  ~SVIPCMutex() {}

#ifndef WIN32
  static bool str2key(const std::string& key, key_t* out) {
    // support decimal, octal, hexadecimal format
    errno = 0;
    int64_t shm_key = strtol(key.c_str(), NULL, 0);
    if (errno)
      return false;
    *out = static_cast<key_t>(shm_key);
    return true;
  }
#endif

  int Init(std::string key) {
#ifdef WIN32
    mutex_handle_ = ::OpenMutex(MUTEX_ALL_ACCESS, FALSE,
                                (LPCSTR)key.c_str());  // 打开进程锁
    if (nullptr == mutex_handle_) {
      printf("[SVIPCMutex::init] Create Mutex\n");
      mutex_handle_ =  CreateMutex(NULL, false, (LPCSTR)key.c_str());  // 创建进程锁
    }
    if (nullptr == mutex_handle_) {
      printf("[SVIPCMutex::init] Create error: %s\n", strerror(errno));
      return -1;
    }
#else
#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
    /* union semun is defined by including <sys/sem.h> */
#else
    /* according to X/OPEN we have to define it ourselves */
    union semun {
      int val;               /* value for SETVAL */
      struct semid_ds *buf;  /* buffer for IPC_STAT, IPC_SET */
      unsigned short *array; /* array for GETALL, SETALL Linux specific part: */
      struct seminfo *__buf; /* buffer for IPC_INFO */
    };
#endif
    key_t shm_key;
    if (!str2key(key, &shm_key)) {
      printf("[XShmRWMutex::init] string 2 key failed.\n");
      return -1;
    }

    union semun arg;
    u_short array[2] = { 0, 0 };
    // 生成信号量集, 包含两个信号量
    if ((mutex_handle_ = semget(shm_key, 2, IPC_CREAT | IPC_EXCL | 0666)) != -1) {
      arg.array = &array[0];

      // 将所有信号量的值设置为0
      if (semctl(mutex_handle_, 0, SETALL, arg) == -1) {
        printf("[XShmRWMutex::init] semctl error: %s\n", strerror(errno));
        return -1;
      }
    } else {
      // 信号量已经存在
      if (errno != EEXIST) {
        printf("[XShmRWMutex::init] sem has exist error: %s\n", strerror(errno));
        return -2;
      }

      // 连接信号量
      if ((mutex_handle_ = semget(shm_key, 2, 0666)) == -1) {
        printf("[XShmRWMutex::init] connect sem error: %s\n", strerror(errno));
      }
    }
#endif
    return 0;
  }

  int lock() const {
#ifdef WIN32
    DWORD ts = WaitForSingleObject(mutex_handle_, INFINITE);  // 获取进程锁
    if (ts > 0) {
      return 1;
    }
    return -1;
#else   // WIN32
    //进入排他锁, 第一个信号量和第二个信号都没有被使用过(即, 两个锁都没有被使用)
    //等待第一个信号量变为0
    //等待第二个信号量变为0
    //释放第一个信号量(第一个信号量+1, 表示有一个进程使用第一个信号量)
    struct sembuf sops[3] = {
      {0, 0, SEM_UNDO}, {1, 0, SEM_UNDO}, {0, 1, SEM_UNDO}
    };
    size_t nsops = 3;

    int ret = -1;
    do {
      ret = semop(mutex_handle_, &sops[0], nsops);
    } while ((ret == -1) && (errno == EINTR));

    return ret;
#endif  // WIN32
  }

  int unlock() const {
#ifdef WIN32
    BOOL bRet = ReleaseMutex(mutex_handle_);  //释放互斥量
    if (TRUE == bRet) {
      return 1;
    }
    return -1;
#else   // WIN32
    //解除排他锁, 有进程使用过第一个信号量
    //等待第一个信号量(信号量值>=1)
    struct sembuf sops[1] = { {0, -1, SEM_UNDO} };
    size_t nsops = 1;

    int ret = -1;
    do {
      ret = semop(mutex_handle_, &sops[0], nsops);
    } while ((ret == -1) && (errno == EINTR));

    return ret;
#endif  // WIN32
  }

  bool trylock() const {
#ifndef WIN32
    struct sembuf sops[3] = { {0, 0, SEM_UNDO | IPC_NOWAIT},
      {1, 0, SEM_UNDO | IPC_NOWAIT},
      {0, 1, SEM_UNDO | IPC_NOWAIT}
    };
    size_t nsops = 3;

    int nret = semop(mutex_handle_, &sops[0], nsops);
    if (nret == -1) {
      if (errno == EAGAIN) {
        //无法获得锁
        return false;
      } else {
        printf("[XShmRWMutex::trywlock] semop error : %s\n", strerror(errno));
      }
    }
#endif  // WIN32
    return true;
  }

 private:
#ifdef WIN32
  HANDLE mutex_handle_;
#else
  int    mutex_handle_;
#endif
};

class SVIPCScope {
 public:
  SVIPCScope(SVIPCMutex *mutex) 
    : mutex_(mutex) {
    mutex_->lock();
  }
  ~SVIPCScope() {
    mutex_->unlock();
  }

 private:
  SVIPCMutex *mutex_;
};

}  // namespace impl

using SVIPC_HugeTLB = impl::SVIPCShmAlloc<true>;
using SVIPC         = impl::SVIPCShmAlloc<false>;
using SVIPCMutex    = impl::SVIPCMutex;
using SVIPCScope    = impl::SVIPCScope;

}  // namespace shmc

#endif  // SHMC_SVIPC_SHM_ALLOC_H_
