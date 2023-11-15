#ifndef TIGER_TIMER_H
#define TIGER_TIMER_H
#include "heap_timer.h"
#include "wheel_timer.h"

template <class T = WheelTimer>
class Timer
{
public:
    void DetectTimers()
    {
//        std::lock_guard<std::recursive_mutex> guard(mutex_);

        timer_iml_.DetectTimers();
    }
    /***
     * 注册定时器函数
     * @param time_out_func 定时器超时函数，参数为当前时间戳（毫秒）
     * @param interval 定时器间隔(毫秒)
     * @param delay 定时器首次超时据当前的时间差(毫秒),0为采用间隔时间
     * @param expired_times 需要超时的次数，次数消耗完后自动取消，０为无限重复
     * @return timerId
     */
    int register_timer(std::function<void(uint64_t)> time_out_func, unsigned int interval, unsigned int delay, int expired_times)
    {
//        std::lock_guard<std::recursive_mutex> guard(mutex_);
        return timer_iml_.register_timer(time_out_func, interval, delay, expired_times);
    }
    void unregister_timer(int timerId)
    {
//        std::lock_guard<std::recursive_mutex> guard(mutex_);
        timer_iml_.unregister_timer(timerId);
    }

private:
    T timer_iml_;
//    std::recursive_mutex mutex_;

};

#endif