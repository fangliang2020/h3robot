#ifndef EVENTSERVICES_BASE_CYCLYQUEUE_H_
#define EVENTSERVICES_BASE_CYCLYQUEUE_H_

namespace chml {

template <typename T, int32 S = 10>
class CycleQueue {
public:
  CycleQueue()
    : head_(0)
    , tail_(0){
  }
  ~CycleQueue() {
  }

  //判断空
  bool empty() {
    return head_ == tail_;
  }

  //返回个数
  int size() {
    return (tail_ - head_ + S) % S;
  }

  //入队
  bool push(T a, bool force = false) {
    if ((tail_ - head_ + S) % S == S - 1) {
      if (force) head_ = (head_ + 1) % S;
      else return false;
    }
    queue_[tail_] = a;
    tail_ = (tail_ + 1) % S;
    return true;
  }

  //出队
  bool pop() {
    if ((tail_ - head_ + S) % S == 0) {
      return false;
    }
    head_ = (head_ + 1) % S;
    return true;
  }

  //队首
  T top() {
    return queue_[head_ % S];
  }

private:
  T queue_[S];
  int32 head_;//指向队首
  int32 tail_;//指向队尾
};

}

#endif