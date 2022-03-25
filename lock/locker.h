#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>
//封装线程间同步机制：信号量，互斥锁和信号变量
class sem
{
private:
    sem_t m_sem;//信号量标识
public:
    sem(int num=0)
    {
        //参数：标识，进程/线程，初始值
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }
};
class locker
{
private:
    pthread_mutex_t m_mutex;//互斥锁标识
public:
    locker()
    {
        //参数：标识，互斥锁属性，NULL为默认值
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    //用于条件变量中互斥锁标识的获取
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
};
class cond
{
private:
    pthread_cond_t m_cond;
public:
    cond()
    {
        //参数：标识，条件变量属性，NULL默认
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            throw std::exception();
        }
    }
    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        //互斥锁解锁，生产者生产资源
        ret = pthread_cond_wait(&m_cond, m_mutex);
        return ret == 0;
    }
    //如超时或者收到信号，唤醒线程，利用互斥锁封装中的get获取互斥锁标识
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t)
    {
        int ret = 0;
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        return ret == 0;
    }
    //唤醒一个沉睡的线程
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
    //唤醒所有沉睡的线程
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }
     ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }
};
#endif
