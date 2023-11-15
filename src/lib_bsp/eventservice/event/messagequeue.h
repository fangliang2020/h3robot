#ifndef EVENTSERVICE_EVENT_MESSAGEQUEUE_H_
#define EVENTSERVICE_EVENT_MESSAGEQUEUE_H_

#include <algorithm>
#include <cstring>
#include <list>
#include <queue>
#include <vector>

#include "eventservice/base/basictypes.h"
#include "eventservice/base/constructormagic.h"
#include "eventservice/base/criticalsection.h"
#include "eventservice/base/timeutils.h"
// #include "eventservice/base/scoped_ptr.h"
// #include "eventservice/base/scoped_ref_ptr.h"
#include "eventservice/base/sigslot.h"
#include "eventservice/base/socketserver.h"
// #include "eventservice/base/shared_ptr.h"

#include "eventservice/event/messagehandler.h"
#include "boost/boost_settings.hpp"

namespace chml {

struct Message;
class MessageQueue;

// MessageQueueManager does cleanup of of message queues

// 消息队列管理器
// 此类是一个全局单例类，用于管理消息队列
class MessageQueueManager {
 public:
  // 添加消息队列
  // message_queue:消息队列
  static void Add(MessageQueue *message_queue);
  // 删除消息队列
  // message_queue:消息队列
  static void Remove(MessageQueue *message_queue);
  static void Clear(MessageHandler *handler);

  // For testing purposes, we expose whether or not the MessageQueueManager
  // instance has been initialized. It has no other use relative to the rest of
  // the functions of this class, which auto-initialize the underlying
  // MessageQueueManager instance when necessary.
  static bool IsInitialized();

 private:
  // 获取MessageQueueManager实例，若已经创建,直接返回其实例
  static MessageQueueManager* Instance();

  MessageQueueManager();
  ~MessageQueueManager();

  void AddInternal(MessageQueue *message_queue);
  void RemoveInternal(MessageQueue *message_queue);
  void ClearInternal(MessageHandler *handler);

  static MessageQueueManager* instance_; // 消息队列管理器实例
  // This list contains 'active' MessageQueues.
  std::vector<MessageQueue *> message_queues_; // 消息队列
  CriticalSection crit_; // 线程互斥锁
};

// Derive from this for specialized data
// App manages lifetime, except when messages are purged

class MessageData : public boost::enable_shared_from_this<MessageData> {
 public:
  typedef boost::shared_ptr<MessageData> Ptr;
  MessageData() {}
  virtual ~MessageData() {}
};

template <class T>
class TypedMessageData : public MessageData {
 public:
  typedef boost::shared_ptr< TypedMessageData<T> > Ptr;
  explicit TypedMessageData(const T& data) : data_(data) { }
  const T& data() const {
    return data_;
  }
  T& data() {
    return data_;
  }
 private:
  T data_;
};

// // Like TypedMessageData, but for pointers that require a delete.
// template <class T>
// class ScopedMessageData : public MessageData {
//  public:
//   typedef boost::shared_ptr< ScopedMessageData<T> > Ptr;
//   explicit ScopedMessageData(T* data) : data_(data) { }
//   const scoped_ptr<T>& data() const {
//     return data_;
//   }
//   scoped_ptr<T>& data() {
//     return data_;
//   }
//  private:
//   scoped_ptr<T> data_;
// };
//
// // Like ScopedMessageData, but for reference counted pointers.
// template <class T>
// class ScopedRefMessageData : public MessageData {
//  public:
//   typedef boost::shared_ptr< ScopedRefMessageData<T> > Ptr;
//   explicit ScopedRefMessageData(T* data) : data_(data) { }
//   const scoped_refptr<T>& data() const {
//     return data_;
//   }
//   scoped_refptr<T>& data() {
//     return data_;
//   }
//  private:
//   scoped_refptr<T> data_;
// };
//
template<class T>
inline MessageData* WrapMessageData(const T& data) {
  return new TypedMessageData<T>(data);
}

template<class T>
inline const T& UseMessageData(MessageData* data) {
  return static_cast< TypedMessageData<T>* >(data)->data();
}

template<class T>
inline const T& UseMessageDataPtr(MessageData::Ptr &data) {
  return boost::dynamic_pointer_cast<TypedMessageData<T>, MessageData>(data)->data();
}

template<class T>
class DisposeData : public MessageData {
 public:
  typedef boost::shared_ptr< DisposeData<T> > Ptr;
  explicit DisposeData(T* data) : data_(data) { }
  virtual ~DisposeData() {
    delete data_;
  }
 private:
  T* data_;
};

const uint32 MQID_ANY = static_cast<uint32>(-1);
const uint32 MQID_DISPOSE = static_cast<uint32>(-2);

// No destructor

struct Message {
  Message() {
    //memset(this, 0, sizeof(*this));
    phandler = nullptr;
    message_id = 0;
    ts_sensitive = 0;
  }
  ~Message() {
    pdata.reset();
  }
  typedef boost::shared_ptr<Message> Ptr;
  inline bool Match(MessageHandler* handler, uint32 id) const {
    return (handler == NULL || handler == phandler)
           && (id == MQID_ANY || id == message_id);
  }
  MessageHandler *phandler;
  uint32 message_id;
  MessageData::Ptr pdata;
  uint32 ts_sensitive;
};

typedef std::list<Message> MessageList;

// DelayedMessage goes into a priority queue, sorted by trigger time.  Messages
// with the same trigger time are processed in num_ (FIFO) order.

class DelayedMessage {
 public:
  DelayedMessage(int delay, uint32 trigger, uint32 num, const Message& msg)
    : cmsDelay_(delay), msTrigger_(trigger), num_(num), msg_(msg) { }

  bool operator< (const DelayedMessage& dmsg) const {
    return (dmsg.msTrigger_ < msTrigger_)
           || ((dmsg.msTrigger_ == msTrigger_) && (dmsg.num_ < num_));
  }

  int cmsDelay_;  // for debugging
  uint32 msTrigger_;
  uint32 num_;
  Message msg_;
};

class MessageQueue {
 public:
  explicit MessageQueue(const std::string &namemq, SocketServer* ss = NULL);
  virtual ~MessageQueue();

  SocketServer* socketserver() {
    return ss_;
  }
  void set_socketserver(SocketServer* ss);

  // Note: The behavior of MessageQueue has changed.  When a MQ is stopped,
  // futher Posts and Sends will fail.  However, any pending Sends and *ready*
  // Posts (as opposed to unexpired delayed Posts) will be delivered before
  // Get (or Peek) returns false.  By guaranteeing delivery of those messages,
  // we eliminate the race condition when an MessageHandler and MessageQueue
  // may be destroyed independently of each other.
  virtual void Quit();
  virtual bool IsQuitting();
  virtual void Restart();

  // Get() will process I/O until:
  //  1) A message is available (returns true)
  //  2) cmsWait seconds have elapsed (returns false)
  //  3) Stop() is called (returns false)
  virtual bool Get(Message *pmsg, int cmsWait = kForever,
                   bool process_io = true);
  virtual bool Peek(Message *pmsg, int cmsWait = 0);
  virtual void Post(MessageHandler *phandler,
                    uint32 id = 0,
                    MessageData::Ptr pdata = MessageData::Ptr(),
                    bool time_sensitive = false);
  virtual void PostDelayed(int cmsDelay,
                           MessageHandler* phandler,
                           uint32 id = 0,
                           MessageData::Ptr pdata = MessageData::Ptr());
  virtual void PostAt(uint32 tstamp,
                      MessageHandler *phandler,
                      uint32 id = 0,
                      MessageData::Ptr pdata = MessageData::Ptr());
  virtual void Clear(MessageHandler *phandler,
                     uint32 id = MQID_ANY,
                     MessageList* removed = NULL);
  virtual void Dispatch(Message *pmsg);
  virtual void ReceiveSends();
  // Amount of time until the next message can be retrieved
  virtual int GetDelay();

  bool empty() const {
    return size() == 0u;
  }
  size_t size() const {
    CritScope cs(&crit_);  // msgq_.size() is not thread safe.
    return msgq_.size() + dmsgq_.size() + (fPeekKeep_ ? 1u : 0u);
  }
  void MqStatis(uint32 &mqin,uint32 &mqout);
  // Internally posts a message which causes the doomed object to be deleted
  template<class T> void Dispose(T* doomed) {
    if (doomed) {
      Post(NULL, MQID_DISPOSE, new DisposeData<T>(doomed));
    }
  }

  // When this signal is sent out, any references to this queue should
  // no longer be used.
  sigslot::signal0<> SignalQueueDestroyed;

 protected:
  class PriorityQueue : public std::priority_queue<DelayedMessage> {
   public:
    container_type& container() {
      return c;
    }
    void reheap() {
      make_heap(c.begin(), c.end(), comp);
    }
  };

  void EnsureActive();
  void DoDelayPost(int cmsDelay, uint32 tstamp, MessageHandler *phandler,
                   uint32 id, MessageData::Ptr pdata);

  // The SocketServer is not owned by MessageQueue.
  SocketServer* ss_;
  // If a server isn't supplied in the constructor, use this one.
  boost::scoped_ptr<SocketServer> default_ss_;
  bool fStop_;
  bool fPeekKeep_;
  Message msgPeek_;
  // A message queue is active if it has ever had a message posted to it.
  // This also corresponds to being in MessageQueueManager's global list.
  bool active_;
  MessageList msgq_;
  PriorityQueue dmsgq_;
  uint32 dmsgq_next_num_;
  mutable CriticalSection crit_;
  uint32 msgq_in_;
  uint32 msgq_out_;
 private:
  DISALLOW_COPY_AND_ASSIGN(MessageQueue);
};

}  // namespace chml

#endif  // EVENTSERVICE_EVENT_MESSAGEQUEUE_H_
