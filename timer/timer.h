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

class timer;

// 包含一个client address, 连接的sockfd以及定时器
struct client_data
{
    sockaddr_in address;    // 客户端socket地址
    int sockfd;     // socket文件描述符
    timer *timer;      // 定时器
    // tw_timer *timer;
};

class timer
{
public:
    timer() {}

public:
    time_t expire;  // 超时时间
    // 回调函数
    void (* cb_func)(client_data *);
    client_data *user_data;     // 连接资源
};

class timer_structure
{
public:
    timer_structure(int m_close_log);
    timer_structure();
    ~timer_structure();

    virtual void add_timer(timer *timer) = 0;
    virtual void adjust_timer(timer* timer) = 0;
    virtual void del_timer(timer* timer) = 0;
    virtual void tick() = 0;
};

class Utils
{
public:
    Utils() {}
    
    // Utils(int m_close_log) : m_time_wheel(m_close_log){}
    // Utils(int m_close_log) : m_timer_lst(m_close_log){}

    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    timer_structure m_timer;
    // sort_timer_lst m_timer_lst;
    // time_wheel m_time_wheel;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);

#endif
