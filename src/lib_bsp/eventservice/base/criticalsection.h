#ifndef EVENTSERVICES_BASE_CRITICALSECTION_H__
#define EVENTSERVICES_BASE_CRITICALSECTION_H__

#include "eventservice/base/constructormagic.h"

#ifdef WIN32
#include "eventservice/base/win32.h"
#endif

#ifdef POSIX
#include <pthread.h>
#endif

#ifdef _DEBUG
#define CS_TRACK_OWNER 1
#endif  // _DEBUG

#if CS_TRACK_OWNER
#define TRACK_OWNER(x) x
#else  // !CS_TRACK_OWNER
#define TRACK_OWNER(x)
#endif  // !CS_TRACK_OWNER

namespace chml {

#ifdef WIN32
class CriticalSection {
 public:
  CriticalSection() {
    InitializeCriticalSection(&crit_);
    // Windows docs say 0 is not a valid thread id
    TRACK_OWNER(thread_ = 0);
  }
  ~CriticalSection() {
    DeleteCriticalSection(&crit_);
  }
  void Enter() {
    EnterCriticalSection(&crit_);
    TRACK_OWNER(thread_ = GetCurrentThreadId());
  }
  bool TryEnter() {
    if (TryEnterCriticalSection(&crit_) != FALSE) {
      TRACK_OWNER(thread_ = GetCurrentThreadId());
      return true;
    }
    return false;
  }
  void Leave() {
    TRACK_OWNER(thread_ = 0);
    LeaveCriticalSection(&crit_);
  }

#if CS_TRACK_OWNER
  bool CurrentThreadIsOwner() const {
    return thread_ == GetCurrentThreadId();
  }
#endif  // CS_TRACK_OWNER

 private:
  CRITICAL_SECTION crit_;
  TRACK_OWNER(DWORD thread_);  // The section's owning thread id
};
#endif // WIN32

#ifdef POSIX
class CriticalSection {
 public:
  CriticalSection() {
    pthread_mutexattr_t mutex_attribute;
    pthread_mutexattr_init(&mutex_attribute);
    //Set the "recursive" attribute, which allows a thread to repeatedly lock a mutex 
    //without causing a deadlock, But need to unlock the same number of times.
    pthread_mutexattr_settype(&mutex_attribute, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex_, &mutex_attribute);
    pthread_mutexattr_destroy(&mutex_attribute);
    TRACK_OWNER(thread_ = 0);
  }
  ~CriticalSection() {
    pthread_mutex_destroy(&mutex_);
  }
  void Enter() {
    pthread_mutex_lock(&mutex_);
    TRACK_OWNER(thread_ = pthread_self());
  }
  bool TryEnter() {
    if (pthread_mutex_trylock(&mutex_) == 0) {
      TRACK_OWNER(thread_ = pthread_self());
      return true;
    }
    return false;
  }
  void Leave() {
    TRACK_OWNER(thread_ = 0);
    pthread_mutex_unlock(&mutex_);
  }

#if CS_TRACK_OWNER
  bool CurrentThreadIsOwner() const {
    return pthread_equal(thread_, pthread_self());
  }
#endif  // CS_TRACK_OWNER

 private:
  pthread_mutex_t mutex_;
  TRACK_OWNER(pthread_t thread_);
};
#endif // POSIX

// CritScope, for serializing execution through a scope.
class CritScope {
 public:
  explicit CritScope(CriticalSection *pcrit) {
    pcrit_ = pcrit;
    if(pcrit_) {
      pcrit_->Enter();
    }
  }
  ~CritScope() {
    if(pcrit_) {
      pcrit_->Leave();
    }
  }
  
 private:
  CriticalSection *pcrit_;
  DISALLOW_COPY_AND_ASSIGN(CritScope);
};

// Tries to lock a critical section on construction via
// CriticalSection::TryEnter, and unlocks on destruction if the
// lock was taken. Never blocks.
//
// IMPORTANT: Unlike CritScope, the lock may not be owned by this thread in
// subsequent code. Users *must* check locked() to determine if the
// lock was taken. If you're not calling locked(), you're doing it wrong!
class TryCritScope {
 public:
  explicit TryCritScope(CriticalSection *pcrit) {
    pcrit_ = pcrit;
    if(pcrit_) {
      locked_ = pcrit_->TryEnter();
    }
  }
  ~TryCritScope() {
    if (locked_ && pcrit_) {
      pcrit_->Leave();
    }
  }
  bool locked() const {
    return locked_;
  }
 private:
  bool locked_;
  CriticalSection *pcrit_;
  DISALLOW_COPY_AND_ASSIGN(TryCritScope);
};

// TODO: Replace with platform-specific "atomic" ops.
// Something like: google3/base/atomicops.h TODO: And, move
// it to atomicops.h, which can't be done easily because of complex
// compile rules.
class AtomicOps {
 public:
#ifdef WIN32
  // Assumes sizeof(int) == sizeof(LONG), which it is on Win32 and Win64.
  static int Increment(int* i) {
    return ::InterlockedIncrement(reinterpret_cast<LONG*>(i));
  }
  static int Decrement(int* i) {
    return ::InterlockedDecrement(reinterpret_cast<LONG*>(i));
  }
#else
  static int Increment(int* i) {
    // Could be faster, and less readable:
    // static CriticalSection* crit = StaticCrit();
    // CritScope scope(crit);
    CritScope scope(StaticCrit());
    return ++(*i);
  }

  static int Decrement(int* i) {
    // Could be faster, and less readable:
    // static CriticalSection* crit = StaticCrit();
    // CritScope scope(crit);
    CritScope scope(StaticCrit());
    return --(*i);
  }

 private:
  static CriticalSection* StaticCrit() {
    static CriticalSection* crit = new CriticalSection();
    return crit;
  }
#endif
};

} // namespace chml

#endif // EVENTSERVICES_BASE_CRITICALSECTION_H__
