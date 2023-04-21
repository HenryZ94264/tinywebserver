#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <iostream>
// #include "lst_timer.h"
#include <pthread.h>

#define BUFFER_SIZE 64  

class tw_timer;

// // 包含一个client address, 连接的sockfd以及定时器
// struct client_data
// {
//     sockaddr_in address;    // 客户端socket地址
//     int sockfd;     // socket文件描述符
//     // util_timer *timer;      // 定时器
//     tw_timer *timer;
// };

// 定时器类
class tw_timer
{
public:
    tw_timer(int rot, int ts): prev(nullptr), next(nullptr), rotation(rot), time_slot(ts) {};

public:
    // 回调函数
    void (* cb_func)(client_data*);
    client_data *user_data;     // 客户数据
    tw_timer *prev;
    tw_timer *next;
    int rotation;   // 记录定时器在时间轮转多少圈后生效
    int time_slot;  // 记录定时器属于时间轮上哪个槽

};

class time_wheel
{
public:
    time_wheel(int m_close_log) : cur_slot(0), m_close_log(m_close_log)
    {
        for(int i = 0; i < N; i++)
        {
            slots[i] = nullptr;     // 初始化每个槽节点
        }
    }
    
    time_wheel(): cur_slot(0), m_close_log(1)
    {
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

    // 根据定时值timeout创建一个定时器，并插入相应的槽中
    tw_timer* add_timer(int timeout, client_data* user_data, void (*cb_func)(client_data*))
    {
        if(timeout < 0)
        {
            return nullptr;
        }
        int ticks = ticks < SI ? SI : timeout / SI;
        int rotation = ticks / N;
        int ts = (cur_slot + (ticks % N)) % N;

        tw_timer* timer = new tw_timer(rotation, ts);
        timer->user_data = user_data;
        timer->cb_func = cb_func;
        // LOG_DEBUG("current ts is: %d", ts);
        // cout << "current ts is: " << ts << endl;
        if(slots[ts] == nullptr)
        {
            slots[ts] = timer;
        }
        else
        {
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        return timer;
    }

    // 删除目标定时器
    void del_timer(tw_timer* timer)
    {
        if(!timer)
        {
            return;
        }
        // 定位到ts所在的槽
        int ts = timer->time_slot;
        // 如果目标timer是头节点，则直接删除
        if(timer == slots[ts])
        {
            slots[ts] = timer->next;
            if(slots[ts])
                slots[ts]->prev = NULL;
            delete timer;
        }
        else
        {
            timer->prev->next = timer->next;
            if(timer->next)
            {
                timer->next->prev = timer->prev;
            }
            delete timer;
        }
        
    }

    // 心跳，时间轮向前滚动一个槽间隔
    void tick()
    {
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
    void adjust_timer(tw_timer* timer)
    {
        if(!timer)
        {
            return;
        }
        // tw_timer *tmp = timer->next;
        // 被调整的定时器是头节点
        int ts = timer->time_slot;
        if (slots[ts] == timer)
        {
            slots[ts] = timer->next;
            if(slots[ts])
            {
                timer->next->prev = nullptr;
            }
            timer->next = nullptr;

            // 向后延长15个time slot
            ts += 3;
            ts = (cur_slot + ts) % N;
            timer->time_slot = ts;
            if(slots[ts] == nullptr)
            {
                slots[ts] = timer;
            }
            else
            {
                timer->next = slots[ts];
                slots[ts]->prev = timer;
                slots[ts] = timer;
            }
        }
        // 被调整的定时器节点不是头节点
        else
        {
            timer->prev->next = timer->next;
            if(timer->next)
                timer->next->prev = timer->prev;
            timer->next = nullptr;
            timer->prev = nullptr;

            // 向后延长3个time slot
            ts += 3;
            ts = (cur_slot + ts) % N;
            timer->time_slot = ts;
            if(slots[ts] == nullptr)
            {
                slots[ts] = timer;
            }
            else
            {
                timer->next = slots[ts];
                slots[ts]->prev = timer;
                slots[ts] = timer;
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