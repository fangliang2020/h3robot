#include "eventservice/event/thread.h"

class CThreadTest : public chml::Runnable {
public:
  CThreadTest() {
  }
  ~CThreadTest() {
  }

  virtual void Run(chml::Thread* thread) {
    while (true) {
      printf("minor thread.\n");
      chml::Thread::SleepMs(1000);
    }
  }
};

int main() {
  chml::Thread thread;
  thread.Start(new CThreadTest());
  
  while (true) {
    printf("main thread.\n");
    chml::Thread::SleepMs(2000);
  }
  return 0;
}

