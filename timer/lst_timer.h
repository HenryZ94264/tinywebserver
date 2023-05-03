#ifndef LST_TIMER
#define LST_TIMER

#include "base_timer.h"



// // 包含一个client address, 连接的sockfd以及定时器
// struct client_data
// {
//     sockaddr_in address;    // 客户端socket地址
//     int sockfd;     // socket文件描述符
//     util_timer *timer;      // 定时器
//     // tw_timer *timer;
// };

// 链表定时器，继承自基类定时器类
class list_timer : public base_timer
{
public:
    list_timer() : prev(nullptr), next(NULL) {}
    virtual ~list_timer() override {};
public:
    list_timer *prev;
    list_timer *next;
};

// 通过升序链表来组织定时器，继承自定时器数据结构类
class sort_timer_lst : public timer_structure
{
public:
    sort_timer_lst(int m_close_log);
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(base_timer *timer) override;
    void adjust_timer(base_timer *timer) override;
    void del_timer(base_timer *timer) override;
    void tick() override;

private:
    void add_timer(list_timer *timer, list_timer *lst_head);

    list_timer *head;
    list_timer *tail;

    int m_close_log;
};


#endif
