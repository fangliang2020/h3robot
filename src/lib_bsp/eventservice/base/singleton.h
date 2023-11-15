#ifndef EVENTSERVICE_BASE_SINGLETON_H
#define EVENTSERVICE_BASE_SINGLETON_H

#include "criticalsection.h"

namespace chml {

template <typename T>
class Singleton {
 public:
  static T *GetInstance() {
    if (instance_ == NULL) {
      CritScope cs(&mutex_);
      if (instance_ == NULL) {
        instance_ = new T();
      }
    }
    return instance_;
  }

  static void Destroy() {
    CritScope cs(&mutex_);
    if (instance_ != NULL) {
      delete instance_;
      instance_ = NULL;
    }
  }

 private:
  static CriticalSection   mutex_;
  static T                *instance_;
};

template <typename T>
T *Singleton<T>::instance_ = NULL;

template <typename T>
CriticalSection Singleton<T>::mutex_;

}  // namespace chml

#endif  // EVENTSERVICE_BASE_SINGLETON_H
