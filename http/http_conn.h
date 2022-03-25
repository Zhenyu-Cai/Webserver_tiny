#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
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
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr);//初始化数据
    void close_conn(bool real_close = true);//关闭连接
    void process();//封装了process_read和process_write,成功后注册写事件，准备发送给socket
    bool read_once();//一次性读取数据
    bool write();//封装将缓冲器写进socket的过程
    sockaddr_in *get_address()//获取地址
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);//连接数据库获取账号密码

private:
    void init();//初始化，包含在共有初始化中
    HTTP_CODE process_read();//以状态机的方式实现，根据读缓冲器已有数据，结合函数parse_line，get_line获取行，parse_request_line，parse_headers，parse_content，parse_line读取行信息并执行do_request
    bool process_write(HTTP_CODE ret);//根据process_read返回的状态，决定所写的响应报文
    HTTP_CODE parse_request_line(char *text);//解析头部请求（get，http，url）
    HTTP_CODE parse_headers(char *text);//解析头部数据（connection，host，content_length)
    HTTP_CODE parse_content(char *text);//判断请求报文是否完整
    HTTP_CODE do_request();//根据process_read读取的信息进行定位以确定选择哪一个网页文件，并打开刚文件映射到内存
    char *get_line() { return m_read_buf + m_start_line; };//根据read_once()函数进行分行，并将每一行的结果返回
    LINE_STATUS parse_line();//根据已读取数据，将行连接处置'0''0';
    void unmap();//取消映射网页文件
    bool add_response(const char *format, ...);//封装将信息写入写缓冲器的过程
    bool add_content(const char *content);//调用add_response封装，写入参数内容
    bool add_status_line(int status, const char *title);//调用add_response封装，写入状态行
    bool add_headers(int content_length);//根据数据体长度，是否长连接以及空行这三个函数，写入头部
    bool add_content_type();//调用add_response封装，写入数据体类型
    bool add_content_length(int content_length);//调用add_response封装，写入数据体长度
    bool add_linger();//调用add_response封装，写入是否长连接
    bool add_blank_line();//调用add_response封装，写入换行

public:
    static int m_epollfd;//监听标识符
    static int m_user_count;//连接数量
    MYSQL *mysql;//数据库连接对象

private:
    int m_sockfd;//连接的socket
    sockaddr_in m_address;//客户地址
    char m_read_buf[READ_BUFFER_SIZE];//读缓冲器
    int m_read_idx;//读的指针
    int m_checked_idx;//检查指针，必须比读指针小
    int m_start_line;//每一行开始的位置，在process_read实现
    char m_write_buf[WRITE_BUFFER_SIZE];//写缓冲器
    int m_write_idx;//写指针
    CHECK_STATE m_check_state;//状态
    METHOD m_method;//get or post
    char m_real_file[FILENAME_LEN];//解析url后真实文件路径
    char *m_url;//请求报文信息url
    char *m_version;//请求报文信息http version
    char *m_host;//请求报文信息host
    int m_content_length;//请求报文信息 数据长度
    bool m_linger;//是否长连接
    char *m_file_address;//dorequest映射文件地址
    struct stat m_file_stat;//dorequest文件属性
    struct iovec m_iv[2];//分散内存结构体数组
    int m_iv_count;//结构体数量
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send;//要发送的信息
    int bytes_have_send;//已发送的信息
};

#endif
