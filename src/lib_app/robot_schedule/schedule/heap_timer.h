//
// Created by franck on 17/09/18.
//

#ifndef TIGER_HEAP_TIMER_H
#define TIGER_HEAP_TIMER_H

#include <unordered_map>
#include <vector>
#include <functional>


#define REPEAT_FOREVER 0

class HeapTimer
{
public:
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
    int register_timer(std::function<void(uint64_t)> time_out_func, unsigned int interval, unsigned int delay = 0, int expired_times = REPEAT_FOREVER);
    void unregister_timer(int timerId);

private:
    void UpHeap(size_t index);
    void DownHeap(size_t index);
    void SwapHeap(size_t index1, size_t index2);

private:
    struct HeapEntry
    {
        std::function<void(uint64_t)> func_ptr;
        int timerId_;
        unsigned int interval_;
        unsigned long long expires_;
        int repeat_;

        int heapIndex_;
    };
    std::unordered_map<int, HeapEntry> timers;
    std::vector<HeapEntry*> heap_;
    int timerId_ {0};

};


#endif