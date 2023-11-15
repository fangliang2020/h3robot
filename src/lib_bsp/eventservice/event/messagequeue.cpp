#ifdef POSIX
#include <sys/time.h>
#endif
#include <iostream>

#include "eventservice/event/messagequeue.h"
#include "eventservice/net/networkservice.h"
#include "eventservice/log/log_client.h"

#if defined(ENABLE_MSGQUEUE_DEBUG)
#include <sys/prctl.h>
#include <sys/syscall.h> 

//#define gettaskid() syscall(__NR_gettid)
typedef std::map<chml::MessageQueue *,std::string> CMqMapDebug;
CMqMapDebug g_mamapdebug;
std::string GetMqDebugInfo(){
  std::string allmqinfo;
  std::string onemq;
  CMqMapDebug::iterator it = g_mamapdebug.begin();
  for (;it != g_mamapdebug.end();++it) {
    uint32 incnt = 0;
    uint32 outcnt = 0;
    it->first->MqStatis(incnt,outcnt);
    
    onemq = it->second;//name
    onemq.append(":res:"+std::to_string(it->first->size()));//队列中没有读取的
    onemq.append("\tin:"+std::to_string(incnt)+"\tout:"+std::to_string(outcnt)+"\n");//队列输入输出统计
    allmqinfo.append(onemq);
  }
  return allmqinfo;
}
#endif


namespace chml {


const uint32 kMaxMsgLatency = 150;  // 150 ms

//------------------------------------------------------------------
// MessageQueueManager
MessageQueueManager* MessageQueueManager::instance_ = NULL;

MessageQueueManager* MessageQueueManager::Instance() {
  // Note: This is not thread safe, but it is first called before threads are
  // spawned.
  if (!instance_)
    instance_ = new MessageQueueManager;
  return instance_;
}

bool MessageQueueManager::IsInitialized() {
  return instance_ != NULL;
}

MessageQueueManager::MessageQueueManager() {
}

MessageQueueManager::~MessageQueueManager() {
}

void MessageQueueManager::Add(MessageQueue *message_queue) {
  return Instance()->AddInternal(message_queue);
}
void MessageQueueManager::AddInternal(MessageQueue *message_queue) {
  // MessageQueueManager methods should be non-reentrant, so we
  // ASSERT that is the case.  If any of these ASSERT, please
  // contact bpm or jbeda.
  ASSERT(!crit_.CurrentThreadIsOwner());
  CritScope cs(&crit_);
  message_queues_.push_back(message_queue);
}

void MessageQueueManager::Remove(MessageQueue *message_queue) {
  // If there isn't a message queue manager instance, then there isn't a queue
  // to remove.
  if (!instance_) return;
  return Instance()->RemoveInternal(message_queue);
}
void MessageQueueManager::RemoveInternal(MessageQueue *message_queue) {
  ASSERT(!crit_.CurrentThreadIsOwner());  // See note above.
  // If this is the last MessageQueue, destroy the manager as well so that
  // we don't leak this object at program shutdown. As mentioned above, this is
  // not thread-safe, but this should only happen at program termination (when
  // the ThreadManager is destroyed, and threads are no longer active).
  bool destroy = false;
  {
    CritScope cs(&crit_);
    std::vector<MessageQueue *>::iterator iter;
    iter = std::find(message_queues_.begin(), message_queues_.end(),
                     message_queue);
    if (iter != message_queues_.end()) {
      message_queues_.erase(iter);
    }
    destroy = message_queues_.empty();
  }
  if (destroy) {
    instance_ = NULL;
    delete this;
  }
}

void MessageQueueManager::Clear(MessageHandler *handler) {
  // If there isn't a message queue manager instance, then there aren't any
  // queues to remove this handler from.
  if (!instance_) return;
  return Instance()->ClearInternal(handler);
}
void MessageQueueManager::ClearInternal(MessageHandler *handler) {
  ASSERT(!crit_.CurrentThreadIsOwner());  // See note above.
  CritScope cs(&crit_);
  std::vector<MessageQueue *>::iterator iter;
  for (iter = message_queues_.begin(); iter != message_queues_.end(); iter++)
    (*iter)->Clear(handler);
}

//------------------------------------------------------------------
// MessageQueue

MessageQueue::MessageQueue(const std::string &namemq,SocketServer* ss)
  : ss_(ss), fStop_(false), fPeekKeep_(false), active_(false),
    dmsgq_next_num_(0) {
  if (!ss_) {
    // Currently, MessageQueue holds a socket server, and is the base class for
    // Thread.  It seems like it makes more sense for Thread to hold the socket
    // server, and provide it to the MessageQueue, since the Thread controls
    // the I/O model, and MQ is agnostic to those details.  Anyway, this causes
    // messagequeue_unittest to depend on network libraries... yuck.
    default_ss_.reset(new NetworkService());
    ss_ = default_ss_.get();
  }
  msgq_in_ = 0;
  msgq_out_ = 0;
  ss_->SetMessageQueue(this);
  #if defined(ENABLE_MSGQUEUE_DEBUG) 
  //char cThreadName[64] = {0};
  //prctl(PR_GET_NAME, (unsigned long)cThreadName);
  g_mamapdebug[this] = namemq;///gettaskid();
  #endif
}

MessageQueue::~MessageQueue() {
  #if defined(ENABLE_MSGQUEUE_DEBUG)
  g_mamapdebug.erase(this);
  #endif

  // The signal is done from here to ensure
  // that it always gets called when the queue
  // is going away.
  SignalQueueDestroyed();
  if (active_) {
    MessageQueueManager::Remove(this);
    Clear(NULL);
  }
  if (ss_) {
    ss_->SetMessageQueue(NULL);
  }
  
}

void MessageQueue::set_socketserver(SocketServer* ss) {
  ss_ = ss ? ss : default_ss_.get();
  ss_->SetMessageQueue(this);
}

void MessageQueue::Quit() {
  DLOG_INFO(MOD_EB, "Message queue quit");
  fStop_ = true;
  ss_->WakeUp();
}

bool MessageQueue::IsQuitting() {
  return fStop_;
}

void MessageQueue::Restart() {
  fStop_ = false;
}

bool MessageQueue::Peek(Message *pmsg, int cmsWait) {
  if (fPeekKeep_) {
    *pmsg = msgPeek_;
    return true;
  }
  if (!Get(pmsg, cmsWait))
    return false;
  msgPeek_ = *pmsg;
  fPeekKeep_ = true;
  return true;
}

bool MessageQueue::Get(Message *pmsg, int cmsWait, bool process_io) {
  // Return and clear peek if present
  // Always return the peek if it exists so there is Peek/Get symmetry
  //DLOG_INFO(MOD_EB, "Debug Message");
  if (fPeekKeep_) {
    *pmsg = msgPeek_;
    fPeekKeep_ = false;
    return true;
  }

  // Get w/wait + timer scan / dispatch + socket / event multiplexer dispatch

  int cmsTotal = cmsWait;
  int cmsElapsed = 0;
  uint32 msStart = Time();
  uint32 msCurrent = msStart;
  while (true) {
    // Check for sent messages
    ReceiveSends();

    // Check for posted events
    int cmsDelayNext = kForever;
    bool first_pass = true;
    while (true) {
      // All queue operations need to be locked, but nothing else in this loop
      // (specifically handling disposed message) can happen inside the crit.
      // Otherwise, disposed MessageHandlers will cause deadlocks.
      {
        CritScope cs(&crit_);
        // On the first pass, check for delayed messages that have been
        // triggered and calculate the next trigger time.
        if (first_pass) {
          first_pass = false;
          while (!dmsgq_.empty()) {
            if (TimeIsLater(msCurrent, dmsgq_.top().msTrigger_)) {
              cmsDelayNext = TimeDiff32(dmsgq_.top().msTrigger_, msCurrent);
              break;
            }
            msgq_.emplace_back(dmsgq_.top().msg_);
            msgq_in_ += 1;
            dmsgq_.pop();
          }
        }
        // Pull a message off the message queue, if available.
        if (msgq_.empty()) {
          break;
        } else {
          *pmsg = msgq_.front();
          msgq_out_ += 1;
          msgq_.pop_front();
        }
      }  // crit_ is released here.

      // Log a warning for time-sensitive messages that we're late to deliver.
      if (pmsg->ts_sensitive) {
        int32 delay = TimeDiff32(msCurrent, pmsg->ts_sensitive);
        if (delay > 0) {
          DLOG_WARNING(MOD_EB, "id: %d, delay:%d(ms)",
                       pmsg->message_id, (delay + kMaxMsgLatency));
        }
      }
      // If this was a dispose message, delete it and skip it.
      if (MQID_DISPOSE == pmsg->message_id) {
        ASSERT(NULL == pmsg->phandler);
        //delete pmsg->pdata;
        *pmsg = Message();
        continue;
      }
      return true;
    }

    if (fStop_)
      break;

    // Which is shorter, the delay wait or the asked wait?

    int cmsNext;
    if (cmsWait == kForever) {
      cmsNext = cmsDelayNext;
    } else {
      cmsNext = _max(0, cmsTotal - cmsElapsed);
      if ((cmsDelayNext != kForever) && (cmsDelayNext < cmsNext))
        cmsNext = cmsDelayNext;
    }

    // Wait and multiplex in the meantime
    if (!ss_->Wait(cmsNext, process_io)) {
      DLOG_ERROR(MOD_EB, "Wait failed");
      return false;
    }

    // If the specified timeout expired, return
    msCurrent = Time();
    cmsElapsed = TimeDiff32(msCurrent, msStart);
    if (cmsWait != kForever) {
      if (cmsElapsed >= cmsWait)
        return false;
    }
  }
  return false;
}

void MessageQueue::ReceiveSends() {
}

void MessageQueue::Post(MessageHandler *phandler,
                        uint32 id,
                        MessageData::Ptr pdata,
                        bool time_sensitive) {
  if (fStop_)
    return;

  // Keep thread safe
  // Add the message to the end of the queue
  // Signal for the multiplexer to return

  CritScope cs(&crit_);
  EnsureActive();
  Message msg;
  msg.phandler = phandler;
  msg.message_id = id;
  msg.pdata = pdata;
  if (time_sensitive) {
    msg.ts_sensitive = Time() + kMaxMsgLatency;
  }
  msgq_.push_back(msg);
  msgq_in_ += 1;
  ss_->WakeUp();
}

void MessageQueue::PostDelayed(int cmsDelay, MessageHandler* phandler, uint32 id, MessageData::Ptr pdata) {
  return DoDelayPost(cmsDelay, TimeAfter(cmsDelay), phandler, id, pdata);
}

void MessageQueue::PostAt(uint32 tstamp, MessageHandler* phandler, uint32 id, MessageData::Ptr pdata) {
  return DoDelayPost(TimeUntil(tstamp), tstamp, phandler, id, pdata);
}

void MessageQueue::DoDelayPost(int cmsDelay,
                               uint32 tstamp,
                               MessageHandler *phandler,
                               uint32 id,
                               MessageData::Ptr pdata) {
  if (fStop_)
    return;

  // Keep thread safe
  // Add to the priority queue. Gets sorted soonest first.
  // Signal for the multiplexer to return.

  CritScope cs(&crit_);
  EnsureActive();
  Message msg;
  msg.phandler = phandler;
  msg.message_id = id;
  msg.pdata = pdata;
  DelayedMessage dmsg(cmsDelay, tstamp, dmsgq_next_num_, msg);
  dmsgq_.push(dmsg);

  // If this message queue processes 1 message every millisecond for 50 days,
  // we will wrap this number.  Even then, only messages with identical times
  // will be misordered, and then only briefly.  This is probably ok.
  ML_VERIFY(0 != ++dmsgq_next_num_);

  ss_->WakeUp();
}

int MessageQueue::GetDelay() {
  CritScope cs(&crit_);

  if (!msgq_.empty())
    return 0;

  if (!dmsgq_.empty()) {
    int64 delay = TimeUntil(dmsgq_.top().msTrigger_);
    if (delay < 0)
      delay = 0;
    return delay;
  }

  return kForever;
}

void MessageQueue::Clear(MessageHandler *phandler, uint32 id,
                         MessageList* removed) {
  CritScope cs(&crit_);

  // Remove messages with phandler

  if (fPeekKeep_ && msgPeek_.Match(phandler, id)) {
    if (removed) {
      removed->push_back(msgPeek_);
    } else {
      //delete msgPeek_.pdata;
    }
    fPeekKeep_ = false;
  }

  // Remove from ordered message queue

  for (MessageList::iterator it = msgq_.begin(); it != msgq_.end();) {
    if (it->Match(phandler, id)) {
      if (removed) {
        removed->push_back(*it);
      } else {
        // delete it->pdata;
      }
      it = msgq_.erase(it);
    } else {
      ++it;
    }
  }

  // Remove from priority queue. Not directly iterable, so use this approach

  PriorityQueue::container_type::iterator new_end = dmsgq_.container().begin();
  for (PriorityQueue::container_type::iterator it = new_end;
       it != dmsgq_.container().end(); ++it) {
    if (it->msg_.Match(phandler, id)) {
      if (removed) {
        removed->push_back(it->msg_);
      } else {
        //delete it->msg_.pdata;
      }
    } else {
      *new_end++ = *it;
    }
  }
  dmsgq_.container().erase(new_end, dmsgq_.container().end());
  dmsgq_.reheap();
}

void MessageQueue::Dispatch(Message *pmsg) {
  pmsg->phandler->OnMessage(pmsg);
}

void MessageQueue::EnsureActive() {
  ASSERT(crit_.CurrentThreadIsOwner());
  if (!active_) {
    active_ = true;
    MessageQueueManager::Add(this);
  }
}
void MessageQueue::MqStatis(uint32 &mqin,uint32 &mqout) {
    mqin = msgq_in_;
    mqout = msgq_out_;
}


}  // namespace chml
