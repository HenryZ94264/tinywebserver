#ifndef MY_UTILS
#define MY_UTILS

#include "lst_timer.h"
#include "wheel_timer.h"


class Utils
{
public:
    Utils() {}
    
    // Utils(int m_close_log) : m_time_wheel(m_close_log){}
    // Utils(int m_close_log) : m_timer_lst(m_close_log){}

    ~Utils() 
    {
        delete u_pipefd;
        delete m_timer;
    }

    void init(int timeslot, int timer_type);

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
    timer_structure *m_timer;
    // sort_timer_lst m_timer_lst;
    // time_wheel m_time_wheel;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);


#endif