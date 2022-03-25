#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>
#include "../log/log.h"

class util_timer;
//客户信息结构，包括socket文件描述符，客户端socket地址以及定时器
struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};
//封装定时器（双向链表结构）
class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    //超时时间
    time_t expire;
    //回调函数（删除非活动连接在socket上的注册时间，在main中实现）
    void (*cb_func)(client_data *);
    //客户信息
    client_data *user_data;
    //前定时器
    util_timer *prev;
    //后定时器
    util_timer *next;
};
//定时器容器（双向链表）
class sort_timer_lst
{
public:
    sort_timer_lst() : head(NULL), tail(NULL) {}
    //将链表全部销毁
    ~sort_timer_lst()
    {
        util_timer *tmp = head;
        while (tmp)
        {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }
    //将一个定时器加入容器中，升序排列，头部为过期时间最短
    void add_timer(util_timer *timer)
    {
        if (!timer)
        {
            return;
        }
        if (!head)
        {
            head = tail = timer;
            return;
        }
        if (timer->expire < head->expire)
        {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        //如果要在中间插入，调用私有方法
        add_timer(timer, head);
    }
    //定时器发生变化时，调整其在链表中的位置
    void adjust_timer(util_timer *timer)
    {
        if (!timer)
        {
            return;
        }
        util_timer *tmp = timer->next;
        //原有位置符合要求
        if (!tmp || (timer->expire < tmp->expire))
        {
            return;
        }
        //因更新后，过期时间增大，位置只可能往后移，那么就删除掉这个定时器并重新加入
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
    //删除定时器
    void del_timer(util_timer *timer)
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
    //定时任务处理函数
    void tick()
    {
        //如果容器中不存在定时器
        if (!head)
        {
            return;
        }
        //写入日志
        LOG_INFO("%s", "timer tick");
        //更新日志
        Log::get_instance()->flush();
        time_t cur = time(NULL);
        util_timer *tmp = head;
        while (tmp)
        {
            //还没到时间，先从最小的检查起
            if (cur < tmp->expire)
            {
                break;
            }
            //如果到了就调用回调函数删除客户数据
            tmp->cb_func(tmp->user_data);
            head = tmp->next;
            if (head)
            {
                head->prev = NULL;
            }
            delete tmp;
            tmp = head;
        }
    }

private:
    //添加定时器在容器中以合适的位置
    void add_timer(util_timer *timer, util_timer *lst_head)
    {
        util_timer *prev = lst_head;
        util_timer *tmp = prev->next;
        while (tmp)
        {
            if (timer->expire < tmp->expire)
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
        if (!tmp)
        {
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
    }

private:
    util_timer *head;
    util_timer *tail;
};

#endif
