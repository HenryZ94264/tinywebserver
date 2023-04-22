#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include "base_timer.h"

#define BUFFER_SIZE 64  


// 定时器类
class tw_timer : public base_timer
{
public:
    tw_timer(int rot, int ts): prev(nullptr), next(nullptr), rotation(rot), time_slot(ts) {};
    tw_timer(): prev(nullptr), next(nullptr), rotation(-1), time_slot(-1) {};
    virtual ~tw_timer() override {};
    void reset_tw_timer(const int& rot, const int& ts)
    {
        rotation = rot;
        time_slot = ts;
    }

public:
    tw_timer *prev;
    tw_timer *next;
    int rotation;   // 记录定时器在时间轮转多少圈后生效
    int time_slot;  // 记录定时器属于时间轮上哪个槽
};

class time_wheel : public timer_structure
{
public:
    time_wheel(int m_close_log) : cur_slot(0), m_close_log(m_close_log)
    {
        for(int i = 0; i < N; i++)
        {
            slots[i] = nullptr;     // 初始化每个槽节点
        }
    }
    
    time_wheel(): cur_slot(0), m_close_log(0)
    {
        LOG_DEBUG("using time wheel");
        for(int i = 0; i < N; i++)
        {
            slots[i] = nullptr;
        }
    }

    ~time_wheel()
    {
        // 循环删除每个槽的计时器
        for(int i = 0; i < N; i++)
        {
            tw_timer* p;
            while(p!=nullptr)
            {
                p = slots[i];
                slots[i] = slots[i]->next;
                slots[i]->prev = nullptr;
                p->next = nullptr;
                delete p;
            }
        }
    }
    
    // 创建一个定时器，并插入相应的槽中
    void add_timer(base_timer* timer)
    {
        tw_timer* tmp_timer = dynamic_cast<tw_timer*>(timer);
        int timeout = tmp_timer->expire;
        if(timeout < 0)
        {
            return;
        }

        int ticks = ticks < SI ? SI : timeout / SI;
        int rotation = ticks / N;
        int ts = (cur_slot + (ticks % N)) % N;

        tmp_timer->reset_tw_timer(rotation, ts);
        if(slots[ts] == nullptr)
        {
            slots[ts] = tmp_timer;
        }
        else
        {
            tmp_timer->next = slots[ts];
            slots[ts]->prev = tmp_timer;
            slots[ts] = tmp_timer;
        }
        return;
    }


    // // 根据定时值timeout创建一个定时器，并插入相应的槽中
    // tw_timer* add_timer(client_data* user_data, void (*cb_func)(client_data*))
    // {
    //     if(expire < 0)
    //     {
    //         return nullptr;
    //     }
    //     int ticks = ticks < SI ? SI : timeout / SI;
    //     int rotation = ticks / N;
    //     int ts = (cur_slot + (ticks % N)) % N;

    //     tw_timer* timer = new tw_timer(rotation, ts);
    //     timer->user_data = user_data;
    //     timer->cb_func = cb_func;
    //     // LOG_DEBUG("current ts is: %d", ts);
    //     // cout << "current ts is: " << ts << endl;
    //     if(slots[ts] == nullptr)
    //     {
    //         slots[ts] = timer;
    //     }
    //     else
    //     {
    //         timer->next = slots[ts];
    //         slots[ts]->prev = timer;
    //         slots[ts] = timer;
    //     }
    //     return timer;
    // }

    // 删除目标定时器
    void del_timer(base_timer* timer)
    {
        tw_timer* tmp_timer = dynamic_cast<tw_timer*>(timer);
        if(!tmp_timer)
        {
            return;
        }
        // 定位到ts所在的槽
        int ts = tmp_timer->time_slot;
        // 如果目标timer是头节点，则直接删除
        if(tmp_timer == slots[ts])
        {
            slots[ts] = tmp_timer->next;
            if(slots[ts])
                slots[ts]->prev = NULL;
            delete tmp_timer;
        }
        else
        {
            tmp_timer->prev->next = tmp_timer->next;
            if(tmp_timer->next)
            {
                tmp_timer->next->prev = tmp_timer->prev;
            }
            delete tmp_timer;
        }
        
    }

    // 心跳，时间轮向前滚动一个槽间隔
    void tick()
    {
        LOG_DEBUG("wheel timer tick");
        tw_timer* tmp = slots[cur_slot];
        // LOG_DEBUG("current slot is %d\n", cur_slot);
        while(tmp)
        {
            // LOG_DEBUG("tick the timer once\n");
            // 如果定时器的rotation值大于0，则它在这一轮不起作用
            if(tmp->rotation > 0)
            {
                tmp->rotation--;
                tmp = tmp->next;
            }
            // 否则，说明定时器已经到期，于是执行定时任务，然后删除该定时器
            else
            {
                tmp->cb_func(tmp->user_data);
                if(tmp == slots[cur_slot])
                {
                    // LOG_DEBUG("delete header in cur_slot");
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if(slots[cur_slot])
                    {
                        slots[cur_slot]->prev = nullptr;
                    }
                    tmp = slots[cur_slot];
                }
                else
                {
                    tmp->prev->next = tmp->next;
                    if(tmp->next)
                    {
                        tmp->next->prev = tmp->prev;
                    }
                    tw_timer* tmp2 = tmp->next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        cur_slot = ++cur_slot % N;  // 时间轮转动
    }

    // 调整定时器位置
    void adjust_timer(base_timer* timer)
    {
        tw_timer* tmp_timer = dynamic_cast<tw_timer*>(timer);
        if(!tmp_timer)
        {
            return;
        }
        // tw_timer *tmp = timer->next;
        // 被调整的定时器是头节点
        int ts = tmp_timer->time_slot;
        if (slots[ts] == tmp_timer)
        {
            slots[ts] = tmp_timer->next;
            if(slots[ts])
            {
                tmp_timer->next->prev = nullptr;
            }
            tmp_timer->next = nullptr;

            // 向后延长15个time slot
            ts += 3;
            ts = (cur_slot + ts) % N;
            tmp_timer->time_slot = ts;
            if(slots[ts] == nullptr)
            {
                slots[ts] = tmp_timer;
            }
            else
            {
                tmp_timer->next = slots[ts];
                slots[ts]->prev = tmp_timer;
                slots[ts] = tmp_timer;
            }
        }
        // 被调整的定时器节点不是头节点
        else
        {
            tmp_timer->prev->next = tmp_timer->next;
            if(tmp_timer->next)
                tmp_timer->next->prev = tmp_timer->prev;
            tmp_timer->next = nullptr;
            tmp_timer->prev = nullptr;

            // 向后延长3个time slot
            ts += 3;
            ts = (cur_slot + ts) % N;
            tmp_timer->time_slot = ts;
            if(slots[ts] == nullptr)
            {
                slots[ts] = tmp_timer;
            }
            else
            {
                tmp_timer->next = slots[ts];
                slots[ts]->prev = tmp_timer;
                slots[ts] = tmp_timer;
            }
        }
    }

private:
    // 时间轮上槽的数目
    static const int N = 20;
    // 每SI秒时间轮转动一次，即槽间隔为1s
    static const int SI = 5;
    // 时间轮的槽，其中每个元素指向一个定时器链表，链表无序
    tw_timer* slots[N];
    // 当前时间轮指向的槽
    int cur_slot;
    int m_close_log;
};


#endif