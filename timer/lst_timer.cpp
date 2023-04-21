#include "lst_timer.h"
#include "../http/http_conn.h"

// 默认打开定时器日志
// sort_timer_lst::sort_timer_lst(): m_close_log(0)
sort_timer_lst::sort_timer_lst()
{
    head = NULL;
    tail = NULL;
}

// sort_timer_lst::sort_timer_lst(int m_close_log) : m_close_log(m_close_log)
// {
//     head = NULL;
//     tail = NULL;
// }
sort_timer_lst::~sort_timer_lst()
{
    util_timer *tmp = head;     // 销毁链表
    while (tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}
// 添加定时器
void sort_timer_lst::add_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if (!head)  // 还没有定时器
    {
        head = tail = timer;
        return;
    }
    if (timer->expire < head->expire)   // 定时器的超时小于最小超时
    {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);     // 更复杂的情况调用私有成员调整内部节点
}
// 调整定时器在链表中的位置
void sort_timer_lst::adjust_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    util_timer *tmp = timer->next;
    // 被调整的定时器在链表尾部
    // 定时器超时值仍然小于其下一个位置的超时值，不调整
    if (!tmp || (timer->expire < tmp->expire))
    {
        return;
    }
    // 若定时器是头结点，取出然后插入到链表其他位置
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}
void sort_timer_lst::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if ((timer == head) && (timer == tail))
    {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}
// 定时任务处理函数
void sort_timer_lst::tick()
{
    if (!head)
    {
        return;
    }
    
    time_t cur = time(NULL);    // 获取当前时间
    util_timer *tmp = head;     
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

void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head)
{
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
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

void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
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
    m_timer_lst.tick();
    // m_time_wheel.tick();
    alarm(m_TIMESLOT);      // 周期性地触发SIGALRM信号
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{   
    // 删除在epoll上非活动连接注册的事件
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);      
    close(user_data->sockfd);   // 关闭文件描述符
    http_conn::m_user_count--;  // 减少连接数
}
