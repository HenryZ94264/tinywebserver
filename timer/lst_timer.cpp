#include "lst_timer.h"

// 默认打开定时器日志
// sort_timer_lst::sort_timer_lst(): m_close_log(0)
sort_timer_lst::sort_timer_lst() : head(nullptr), tail(nullptr), m_close_log(0)
{
    LOG_DEBUG("using list timer");
}


sort_timer_lst::~sort_timer_lst()
{
    list_timer *tmp = head;     // 销毁链表
    while (tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}
// 添加定时器
void sort_timer_lst::add_timer (base_timer *timer)
{
    list_timer* tmp_timer =  dynamic_cast<list_timer*>(timer);
    if (!tmp_timer)
    {
        return;
    }
    if (!head)  // 还没有定时器
    {
        head = tail = tmp_timer;
        return;
    }
    if (tmp_timer->expire < head->expire)   // 定时器的超时小于最小超时
    {
        tmp_timer->next = head;
        head->prev = tmp_timer;
        head = tmp_timer;
        return;
    }
    add_timer(tmp_timer, head);     // 更复杂的情况调用私有成员调整内部节点
}
// 调整定时器在链表中的位置
void sort_timer_lst::adjust_timer(base_timer *timer)
{
    list_timer* tmp_timer =  dynamic_cast<list_timer*>(timer);
    if (!tmp_timer)
    {
        return;
    }
    list_timer *tmp = tmp_timer->next;
    // 被调整的定时器在链表尾部
    // 定时器超时值仍然小于其下一个位置的超时值，不调整
    if (!tmp || (tmp_timer->expire < tmp->expire))
    {
        return;
    }
    // 若定时器是头结点，取出然后插入到链表其他位置
    if (tmp_timer == head)
    {
        head = head->next;
        head->prev = NULL;
        tmp_timer->next = NULL;
        add_timer(tmp_timer, head);
    }
    else
    {
        tmp_timer->prev->next = tmp_timer->next;
        tmp_timer->next->prev = tmp_timer->prev;
        add_timer(tmp_timer, tmp_timer->next);
    }
}
void sort_timer_lst::del_timer(base_timer *timer)
{
    list_timer* tmp_timer = dynamic_cast<list_timer*>(timer);
    if (!tmp_timer)
    {
        return;
    }
    if ((tmp_timer == head) && (tmp_timer == tail))
    {
        delete tmp_timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (tmp_timer == head)
    {
        head = head->next;
        head->prev = NULL;
        delete tmp_timer;
        return;
    }
    if (tmp_timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete tmp_timer;
        return;
    }
    tmp_timer->prev->next = tmp_timer->next;
    tmp_timer->next->prev = tmp_timer->prev;
    delete tmp_timer;
}
// 定时任务处理函数
void sort_timer_lst::tick()
{
    LOG_DEBUG("sort list timer tick");
    if (!head)
    {
        return;
    }
    
    time_t cur = time(NULL);    // 获取当前时间
    list_timer *tmp = head;     
    while (tmp)
    {
        if (cur < tmp->expire)      // 只要目前这个定时器的超时时间大于当前时间，则后面的定时器都没有到期
        {
            break;
        }
        tmp->cb_func(tmp->user_data);   // 到期则调用回调函数，执行定时事件
        head = tmp->next;       // 重置头结点
        if (head)
        {
            head->prev = NULL;
        }
        // LOG_DEBUG("timer of sockfd: %d has been deprecated", tmp->user_data->sockfd);

        delete tmp; // 删除被处理的定时器
        tmp = head;
    }
}

void sort_timer_lst::add_timer(list_timer *timer, list_timer *lst_head)
{
    list_timer *prev = lst_head;
    list_timer *tmp = prev->next;
    while (tmp)
    {
        if (timer->expire < tmp->expire)    // 当要插入的timer的超时小于tmp时，插入该位置
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp)   // timer的超时大于链表中所有定时器的超时，则加入到链表尾部
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}
