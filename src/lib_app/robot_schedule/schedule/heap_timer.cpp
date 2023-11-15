//
// Created by franck on 17/09/18.
//

#define _CRT_SECURE_NO_WARNINGS
#include "heap_timer.h"
#ifdef _MSC_VER
# include <sys/timeb.h>
#else
# include <sys/time.h>
#endif


int HeapTimer::register_timer(std::function<void(uint64_t)> time_out_func, unsigned int interval, unsigned int delay, int expired_times)
{
    if(delay == 0)
    {
        delay = interval;
    }
    size_t curHeapIndex = heap_.size();
    int cur_id = ++timerId_;
    auto& entry = timers[cur_id];
    entry.timerId_ = cur_id;
    entry.interval_ = interval;
    entry.expires_ = delay + GetCurrentMillisecs();
    entry.repeat_ = expired_times;
    entry.func_ptr = std::move(time_out_func);
    entry.heapIndex_ = (int)curHeapIndex;

    heap_.emplace_back(&entry);

    UpHeap(curHeapIndex);

    return cur_id;
}

void HeapTimer::unregister_timer(int timerId)
{
    auto iter = timers.find(timerId);
    if(iter != timers.end())
    {
        auto entry = &iter->second;
        size_t curIndex = (size_t)entry->heapIndex_;
        size_t lastIndex = heap_.size() - 1;
        if(curIndex == lastIndex)
        {
            heap_.pop_back();
            timers.erase(iter);
        } else {
            SwapHeap(curIndex, lastIndex);
            heap_.pop_back();
            timers.erase(iter);
            size_t parent = (curIndex - 1) / 2;
            if(curIndex > 0 && heap_[curIndex]->expires_ < heap_[parent]->expires_)
            {
                UpHeap(curIndex);
            } else {
                DownHeap(curIndex);
            }
        }
    }
}

void HeapTimer::DetectTimers()
{
    unsigned long long now = GetCurrentMillisecs();

    std::vector<int> expired_timers;
    while (!heap_.empty() && heap_[0]->expires_ <= now)
    {
        auto entry = heap_[0];
//        entry->func_ptr(now);
        expired_timers.emplace_back(entry->timerId_);
        if(entry->repeat_ > 0)
        {
            if(entry->repeat_ == 1) //last time_out
            {
                size_t lastIndex = heap_.size() - 1;
                if(0 == lastIndex)
                {
                    entry->heapIndex_ = -1;
                    heap_.pop_back();
//                    timers.erase(entry->timerId_);
                } else {
                    SwapHeap(0, lastIndex);
                    entry->heapIndex_ = -1;
                    heap_.pop_back();
//                    timers.erase(entry->timerId_);
                    DownHeap(0);
                }
                continue;
            }
            entry->repeat_--;
        }
        do {
            //make expires time absolute in future
            entry->expires_ += entry->interval_;
        } while(entry->expires_ <= now);

        DownHeap(0);
    }
    //call after check, maybe will cancel timer in call
    for(int timerId : expired_timers)
    {
        auto iter = timers.find(timerId);
        if(iter != timers.end())
        {
            auto& entry = iter->second;
            auto func = entry.func_ptr;
            if(entry.heapIndex_ == -1)
            {
                timers.erase(iter);
            }
            func(now);
        }
    }
}

void HeapTimer::UpHeap(size_t index)
{
    size_t parent = (index - 1) / 2;
    while (index > 0 && heap_[index]->expires_ < heap_[parent]->expires_)
    {
        SwapHeap(index, parent);
        index = parent;
        parent = (index - 1) / 2;
    }
}

void HeapTimer::DownHeap(size_t index)
{
    size_t child = index * 2 + 1;
    while (child < heap_.size())
    {
        size_t minChild = (child + 1 == heap_.size() || heap_[child]->expires_ < heap_[child + 1]->expires_)
                          ? child : child + 1;
        if (heap_[index]->expires_ < heap_[minChild]->expires_)
            break;
        SwapHeap(index, minChild);
        index = minChild;
        child = index * 2 + 1;
    }
}

void HeapTimer::SwapHeap(size_t index1, size_t index2)
{
    auto tmp = heap_[index1];
    heap_[index1] = heap_[index2];
    heap_[index2] = tmp;
    heap_[index1]->heapIndex_ = (int)index1;
    heap_[index2]->heapIndex_ = (int)index2;
}


uint64_t HeapTimer::GetCurrentMillisecs()
{
#ifdef _MSC_VER
    _timeb timebuffer;
    _ftime(&timebuffer);
    unsigned long long ret = timebuffer.time;
    ret = ret * 1000 + timebuffer.millitm;
    return ret;
#else
    timeval tv{};
    ::gettimeofday(&tv, nullptr);
    uint64_t ret = tv.tv_sec;
    return ret * 1000 + tv.tv_usec / 1000;
#endif
}