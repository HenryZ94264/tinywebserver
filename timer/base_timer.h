#ifndef MY_TIMER
#define MY_TIMER

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

class base_timer;   // 前向声明

// 包含一个client address, 连接的sockfd以及定时器
struct client_data
{
    sockaddr_in address;    // 客户端socket地址
    int sockfd;     // socket文件描述符
    base_timer *timer;      // 定时器
};

// 基类定时器，只有一个定时器
class base_timer
{
public:
    base_timer() {};
    virtual ~base_timer() {};

public:
    time_t expire;  // 超时时间
    // 回调函数
    void (* cb_func)(client_data *);
    client_data *user_data;     // 连接资源
};

// 定时器数据结构，用于组织多个定时器
class timer_structure
{
public:
    timer_structure(int m_close_log) {};
    timer_structure() {};
    virtual ~timer_structure() {};

    virtual void add_timer(base_timer *timer) = 0;
    virtual void adjust_timer(base_timer* timer) = 0;
    virtual void del_timer(base_timer* timer) = 0;
    virtual void tick() = 0;
};



#endif
