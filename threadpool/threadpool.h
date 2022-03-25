#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

template <typename T>
class threadpool
{
public:
    //connpool是mysql连接池thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量
    //初始化线程池，设置线程标识数组并分离线程
    threadpool(connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    //删除线程标识数组
    ~threadpool();
    //将一个请求加入请求队列链表中
    bool append(T *request);

private:
    //工作线程运行的函数，它不断从工作队列中取出任务并执行之
    //传入参数为this指针，指代某一线程池对象
    static void *worker(void *arg);
    void run();
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    bool m_stop;                //是否结束线程
    connection_pool *m_connPool;  //数据库连接池对象
};
template <typename T>
threadpool<T>::threadpool( connection_pool *connPool, int thread_number, int max_requests) : m_thread_number(thread_number), m_max_requests(max_requests), m_stop(false), m_threads(NULL),m_connPool(connPool)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    //pthread_t:long int记录子线程标识符
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads)
        throw std::exception();
    for (int i = 0; i < thread_number; i++)
    {
        //第二个参数 null为默认，线程属性设置
        //m_threads+i指向线程数组某一值得指针，静态成员函数worker，没有this指针，因此不会自动传入，不会发生错误，第四个参数所传入的类型为void*
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        //分离线程，成功返回0
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
    m_stop = true;
}
//将请求加入请求队列链表中
template <typename T>
bool threadpool<T>::append(T *request)
{
    m_queuelocker.lock();
    if (m_workqueue.size() > m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
//传入参数为线程池对象得this指针，调用私有函数该对象的私有函数
template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}
//从请求队列中取出一条指令，再从mysql连接池中取出一个连接，调用http类request中的处理函数
template <typename T>
void threadpool<T>::run()
{
    while (!m_stop)
    {
        m_queuestat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request)
            continue;

        //mysql是http类一个共有对象，表示一个可用的mysql连接，通过mysqlcon函数可以从连接池中获取一个可用的连接，当该对象生命周期结束时，自动调用
        //构造函数自动获取mysql连接
        //析构函数释放连接
        connectionRAII mysqlcon(&request->mysql, m_connPool);
        
        request->process();
    }
}
#endif
