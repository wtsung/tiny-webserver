#ifndef WEBSERVER_WEBSERVER_H
#define WEBSERVER_WEBSERVER_H
#include <string>
#include <memory>

#include "epoller.h"
#include "../Timer/timer.h"
#include "../Log/log.h"
#include "../SqlConnPool/sqlconnpool.h"
#include "../SqlConnPool/sqlconnpoolRAII.h"
#include "../SqlConnPool/threadpool.h"
#include "../Http/httpconn.h"

class WebServer {
public:
    WebServer(int port, int trig_mode, int timeout_ms, int opt_linger
              std::string user, std::string password, std::string database_name, int sqlport, 
              int close_log, int sql_num, int thread_num, int log_que_size);
    ~WebServer();

    void start();

private:
    bool _init_socket();
    void _init_event_mode(int trig_mode);
    void _add_client(int fd, sockaddr_in addr);

    void _deal_listen();
    void _deal_read(HttpConn* client);
    void _deal_write(HttpConn* client);

    void _send_error(int fd, const char* info);
    void _extent_time(HttpConn* client);
    void _close_conn(HttpConn* client);

    void _read(HttpConn* client);
    void _write(HttpConn* client);
    void _process(HttpConn* client);

    static int _set_fd_nonblock(int fd);
private:
    static const int MAX_FD = 65536;

    int _port;
    bool _opt_linger;
    int _timeout_ms;
    bool _is_close;
    int _listenfd;
    char* _root_dir;
    int _close_log;

    int _epollfd;

    uint32_t _listen_event;//监听套接字event
    uint32_t _conn_event;

    std::unique_ptr<time_heap> _timer;
    std::unique_ptr<ThreadPool> _threadpool;
    std::unique_ptr<Epoller> _epoller;
    std::unordered_map<int, HttpConn> _users;
};

#endif //WEBSERVER_WEBSERVER_H