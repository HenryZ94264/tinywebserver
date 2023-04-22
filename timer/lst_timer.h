#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
// #include "../log/log.h"
#include "../log/log.h"
#include "base_timer.h"


class list_timer;

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
