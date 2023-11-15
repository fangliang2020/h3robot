#include "wheel_timer.h"

// cpp file //////////////////////////////////////////////////
#define _CRT_SECURE_NO_WARNINGS
#include "Timer.h"
#ifdef _MSC_VER
# include <sys/timeb.h>
#else
# include <sys/time.h>
#endif

#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)
#define OFFSET(N) (TVR_SIZE + (N) *TVN_SIZE)
#define INDEX(V, N) ((V >> (TVR_BITS + (N) *TVN_BITS)) & TVN_MASK)

#include <chrono>

WheelTimer::WheelTimer()
{
    wheels_.resize(TVR_SIZE + 4 * TVN_SIZE);
    checkTime_ = GetCurrentMillisecs();
}

void WheelTimer::DetectTimers()
{
    unsigned long long now = GetCurrentMillisecs();
    while (checkTime_ <= now)
    {
        int index = checkTime_ & TVR_MASK;
        if (!index &&
            !Cascade(OFFSET(0), INDEX(checkTime_, 0)) &&
            !Cascade(OFFSET(1), INDEX(checkTime_, 1)) &&
            !Cascade(OFFSET(2), INDEX(checkTime_, 2)))
        {
            Cascade(OFFSET(3), INDEX(checkTime_, 3));
        }
        ++checkTime_;

        auto& tlist = wheels_[index];
        TimerList temp;
        temp.splice(temp.end(), tlist);
        for (auto entry : temp)
        {
            entry->wheelIndex_ = -1; //make invalid, because old list empty and iter invalid
            entry->func_ptr(now);

            auto iter = timers.find(entry->timerId_); // check timer is exist, maybe canceled in call
            if(iter != timers.end())
            {
                if(entry->repeat_ > 0)
                {
                    if(entry->repeat_ == 1)
                    {
                        timers.erase(iter);
                        continue;
                    }
                    entry->repeat_--;
                }
                entry->expires_ += entry->interval_;
                AddEntryToWheel(entry);
            }
        }
    }
}

int WheelTimer::register_timer(std::function<void(uint64_t)> time_out_func, unsigned int interval, unsigned int delay, int expired_times)
{
    if(delay == 0)
    {
        delay = interval;
    }
    int cur_id = ++timerId_;
    auto& entry = timers[cur_id];
    entry.timerId_ = cur_id;
    entry.interval_ = interval;
    entry.expires_ = delay + GetCurrentMillisecs();
    entry.repeat_ = expired_times;
    entry.func_ptr = std::move(time_out_func);

    AddEntryToWheel(&entry);

    return cur_id;
}

void WheelTimer::unregister_timer(int timerId)
{
    auto iter = timers.find(timerId);
    if (iter != timers.end())
    {
        auto entry = &iter->second;
        if(entry->wheelIndex_ != -1)
        {
            wheels_[entry->wheelIndex_].erase(entry->itr_);
        }
        timers.erase(iter);
    }
}

void WheelTimer::AddEntryToWheel(WheelEntry *entry)
{
    unsigned long long expires = entry->expires_;
    unsigned long long idx = expires - checkTime_;

    if (idx < TVR_SIZE)
    {
        entry->wheelIndex_ = expires & TVR_MASK;
    } else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
        entry->wheelIndex_ = OFFSET(0) + INDEX(expires, 0);
    } else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
        entry->wheelIndex_ = OFFSET(1) + INDEX(expires, 1);
    } else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
        entry->wheelIndex_ = OFFSET(2) + INDEX(expires, 2);
    } else if ((long long) idx < 0) {
        entry->wheelIndex_ = checkTime_ & TVR_MASK;
    } else {
        if (idx > 0xffffffffUL)
        {
            idx = 0xffffffffUL;
            expires = idx + checkTime_;
        }
        entry->wheelIndex_ = OFFSET(3) + INDEX(expires, 3);
    }

    auto& tlist = wheels_[entry->wheelIndex_];
    entry->itr_ = tlist.insert(tlist.end(), entry);
}

int WheelTimer::Cascade(int offset, int index)
{
    auto& tlist = wheels_[offset + index];
    TimerList temp;
    temp.splice(temp.end(), tlist);

    for (auto entry : temp)
    {
        AddEntryToWheel(entry);
    }

    return index;
}

uint64_t WheelTimer::GetCurrentMillisecs()
{
#ifdef _MSC_VER
    _timeb timebuffer;
    _ftime(&timebuffer);
    unsigned long long ret = timebuffer.time;
    ret = ret * 1000 + timebuffer.millitm;
    return ret;
#else
//    timeval tv{};
//    ::gettimeofday(&tv, nullptr);
//    uint64_t ret = tv.tv_sec;
//    return ret * 1000 + tv.tv_usec / 1000;

    std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();

//    auto sec_cnt = std::chrono::time_point_cast<std::chrono::seconds>(current_time).time_since_epoch().count();
//    auto micro_cnt = std::chrono::time_point_cast<std::chrono::microseconds>(current_time).time_since_epoch().count();
//    auto res = sec_cnt * 1000 + micro_cnt / 1000;

    auto milli_cnt = std::chrono::time_point_cast<std::chrono::milliseconds>(current_time).time_since_epoch().count();
//    auto res = sec_cnt * 1000;
    return milli_cnt;
#endif
}