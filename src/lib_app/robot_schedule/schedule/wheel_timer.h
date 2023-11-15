#ifndef TIGER_WHEEL_TIMER_H
#define TIGER_WHEEL_TIMER_H

#include <unordered_map>
#include <list>
#include <vector>
#include <functional>


class WheelTimer
{
public:
    WheelTimer();

    static uint64_t GetCurrentMillisecs();

    void DetectTimers();
    /***
     * 注册定时器函数
     * @param time_out_func 定时器超时函数，参数为当前时间戳（毫秒）
     * @param interval 定时器间隔(毫秒)
     * @param delay 定时器首次超时据当前的时间差(毫秒),0为采用间隔时间
     * @param expired_times 需要超时的次数，次数消耗完后自动取消，０为无限重复
     * @return timerId
     */
    int register_timer(std::function<void(uint64_t)> time_out_func, unsigned int interval, unsigned int delay, int expired_times);
    void unregister_timer(int timerId);

private:
    struct WheelEntry
    {
        int timerId_;
        unsigned interval_;
        unsigned long long expires_;
        int repeat_;
        std::function<void(uint64_t)> func_ptr;

        int wheelIndex_;
        std::list<WheelEntry*>::iterator itr_;
    };

    void AddEntryToWheel(WheelEntry* entry);

    int Cascade(int offset, int index);

private:
    typedef std::list<WheelEntry*> TimerList;
    std::unordered_map<int, WheelEntry> timers;
    std::vector<TimerList> wheels_;
    unsigned long long checkTime_;
    int timerId_ {0};

};


#endif