#include "eventservice/net/eventservice.h"

struct TestMessage : public chml::MessageData {
  typedef boost::shared_ptr<TestMessage> Ptr;
  uint32 index;
};

class TimeEvent : public chml::MessageHandler {
public:
  TimeEvent(chml::EventService::Ptr main_es)
    : main_es_(main_es) {
  }
  virtual ~TimeEvent() {
  }
  void Start() {
    // 创建子线程
    second_es_ = chml::EventService::CreateEventService(NULL, "second_es");
    
    // 主线程投递定时消息
    TestMessage::Ptr tmsg(new TestMessage);
    tmsg->index = 0;
    main_es_->PostDelayed(1000, this, 0, tmsg);
  }

  virtual void OnMessage(chml::Message* msg) {
    TestMessage::Ptr tmsg = boost::dynamic_pointer_cast<TestMessage>(msg->pdata);
    tmsg->index++;
    chml::Thread* current_thread = chml::Thread::Current();
    // 处理主线程定时消息
    if (main_es_->IsThisThread(current_thread)) {
      second_es_->PostDelayed(1000, this, 0, tmsg);
    }
    // 处理子线程定时消息
    else if (second_es_->IsThisThread(current_thread)) {
      main_es_->Post(this, 0, tmsg);
    }
  }
private:
  chml::EventService::Ptr   main_es_;
  chml::EventService::Ptr   second_es_;
};

int main(void) {
  chml::EventService::Ptr main_es
    = chml::EventService::CreateCurrentEventService("MainThread");

  TimeEvent te(main_es);
  te.Start();

  while (1) {
    main_es->Run();
  }
  return EXIT_SUCCESS;
}