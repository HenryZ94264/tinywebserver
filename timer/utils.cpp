#include "utils.h"
#include "../http/http_conn.h"

void Utils::init(int timeslot, int timer_type)
{
    m_TIMESLOT = timeslot;
    if(timer_type == 0)
    {
        sort_timer_lst* timer = new sort_timer_lst();
        m_timer = timer; 
    }
    else if(timer_type == 1)
    {
        time_wheel* timer = new time_wheel();
        m_timer = timer;
    }
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    // 可重入性表示中断后再次进入该函数，环境变量与之前的相同，不会丢失数据
    int save_errno = errno;
    int msg = sig;
    // 将信号值从管道写端写入，传入字符型
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    // 创建sigaction结构体变量
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    // 信号处理函数仅发送信号值，不做对应的逻辑处理
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    // 将所有信号添加到信号集中
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);    // 执行sigaction函数
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer->tick();
    alarm(m_TIMESLOT);      // 周期性地触发SIGALRM信号
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;


void cb_func(client_data *user_data)
{   
    // 删除在epoll上非活动连接注册的事件
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);      
    close(user_data->sockfd);   // 关闭文件描述符
    http_conn::m_user_count--;  // 减少连接数
}
