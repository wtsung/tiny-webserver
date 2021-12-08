#include "webserver.h"

WebServer::WebServer(int port, int trig_mode, int timeout_ms, int opt_linger, 
              std::string user, std::string password, std::string database_name, int sqlport, 
              int close_log, int sql_num, int thread_num, int log_que_size) : 
              _port(port), _opt_linger(opt_linger), _timeout_ms(timeout_ms), _is_close(false), _close_log(close_log)
              _timer(new heap_timer()), _threadpool(new ThreadPool(thread_num)), _epoller(new Epoller())
              {
                  _root_dir = getcwd(nullptr, 256);
                  assert(_root_dir);
                  strncat(_root_dir, "/resourse/", 16);
                  HttpConn::_user_count = 0;
                  HttpConn::_root_dir = _root_dir;
                  SqlConnPool::GetInstance()->Init("localhost", sqlport, user, password, database_name, sql_num);

                  _init_event_mode(trig_mode);
                  if (!_init_socket()) {
                      _is_close = true;
                  }

                  if (!_close_log) {
                      Log::get_instance()->init(".log", 8192, 5000000, 50000);
                      if (_is_close) {
                          LOG_ERROR("========== Server init error!==========");
                      }
                      else {
                          LOG_INFO("========== Server init ==========");
                          LOG_INFO("Port:%d, OpenLinger: %s", _port, _opt_linger ? "true":"false");
                          LOG_INFO("Listen Mode: %s, OpenConn Mode: %s", (_listen_event & EPOLLET ? "ET": "LT"), (_conn_event & EPOLLET ? "ET": "LT"));
                          LOG_INFO("rootDir: %s", HttpConn::_root_dir);
                          LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", sql_num, thread_num);
                      }

                  }
              }
            

WebServer::~WebServer() {
    close(_listenfd);
    _is_close = true;
    free(_root_dir);
    SqlConnPool::GetInstance()->ClosePool();
}

void WebServer::_init_event_mode (int trig_mode) {
    _listen_event = EPOLLRDHUP;
    _conn_event = EPOLLONESHOT | EPOLLRDHUP;
    switch (trig_mode)
    {
    case 0:
        break;
    case 1:
        _conn_event |= EPOLLET;
        break;
    case 2:
        _listen_event |= EPOLLET;
        break;
    case 3:
        _conn_event |= EPOLLET;
        _listen_event |= EPOLLET;
        break;
    default:
        _conn_event |= EPOLLET;
        _listen_event |= EPOLLET;
        break;
    }
    HttpConn::_trig_ET = (_conn_event & EPOLLET);
}

void WebServer::start() {
    int timems = -1;
    if (!_is_close) {
        LOG_INFO("========== Server start ==========");
    } 
    while (!_is_close) {
        if (_timeout_ms > 0){
            timems = _timer->get_next_tick();
        }
        int eventcnt = _epoller->wait(timems);
        for (int i = 0; i < eventcnt; ++i) {
            //事件处理
            int fd = _epoller->get_event_fd(i);
            uint32_t events = _epoller->get_events(i);
            if (fd == _listenfd) {
                _deal_listen();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(_users.count(fd) > 0);
                _close_conn(&_users[fd]);
            }
            else if (events & EPOLLIN) {
                assert(_users.count(fd) > 0);
                _deal_read(&_users[fd]);
            }
            else if (events & EPOLLOUT) {
                assert(_users.count(fd) > 0);
                _deal_write(&_users[fd]);
            }
            else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::_deal_listen() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(_listenfd, (struct sockaddr*)&addr, &len);
        if (fd < 0) {
            return;
        }
        else if (HttpConn::_user_count >= MAX_FD) {
            _send_error(fd, "Server busy!");
            LOG_WARN("CLient is full");
            return;
        }
        _add_client(fd, addr);
    } while (_listen_event & EPOLLET);
}

void WebServer::_deal_read(HttpConn* client) {
    assert(client != nullptr);
    _extent_time(client); //更新
    _threadpool->AddTask(std::bind(&WebServer::_read, this, client));
}

void WebServer::_deal_write(HttpConn* client) {
    assert(client != nullptr);
    _extent_time(client);
    _threadpool->AddTask(std::bind(&WebServer::_write, this, client));
}

void WebServer::_send_error(int fd, const char* info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::_extent_time(HttpConn* client) {
    assert(client != nullptr);
    if (_timeout_ms > 0) {
        _timer->adjust_timer(client, _timeout_ms);
    }
}

void WebServer::_close_conn(HttpConn* client) {
    assert(client != nullptr);
    LOG_INFO("Client[%d] quit!", client->get_fd());
    _epoller->del_fd(client->get_fd());
    client->close_conn();
}

bool WebServer::_init_socket() {
    int ret;
    struct sockaddr_in addr;
    if (_port > 65535 || _port < 1024) {
        LOG_ERROR("Port:%d error!",  _port);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = _port;
    struct linger opt_linger = {0};
    /* 优雅关闭: 直到所剩数据发送完毕或超时 */
    if (_opt_linger) {
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
    }
    _listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenfd < 0) {
        LOG_ERROR("Create socket error!", _port);
        return false;       
    }
    /* 设置优雅关闭 */
    ret = setsockopt(_listenfd, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger));
    if (ret < 0) {
        close(_listenfd);
        LOG_ERROR("Init linger error");
        return false;
    }

    int optval = 1;
    /* 端口复用, 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(_listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if (ret == -1) {
        close(_listenfd);
        LOG_ERROR("set socket setsockopt error !");
        return false;
    }

    ret = bind(_listenfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        close(_listenfd);
        LOG_ERROR("Bind Port:%d error!", _port);
        return false;
    }

    ret = listen(_listenfd, 6);
    if (ret < 0) {
        close(_listenfd);
        LOG_ERROR("Listen port:%d error!", _port);
        return false;
    }

    ret = _epoller->add_fd(_listenfd, _listen_event | EPOLLIN);
    if (ret == 0) {
        close(_listenfd);
        LOG_ERROR("Add listen error!");
        return false;
    }
    //设置为非阻塞
    _set_fd_nonblock(_listenfd);
    LOG_INFO("Server port:%d", _port);
    return true;
}

int WebServer::_set_fd_nonblock (int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}



void WebServer::_add_client(int fd, sockaddr_in addr) {
    assert(fd > 0);
    _users[fd].init(fd, addr);//初始化httpconn
    //添加定时器
    if (_timeout_ms > 0) {
        _timer->add_timer(fd, _timeout_ms, std::bind(&WebServer::_close_conn, this, &_users[fd]));
    }
    _epoller->add_fd(fd, EPOLLIN | _conn_event);
    _set_fd_nonblock(fd);
    LOG_INFO("Client[%d] in!", _users[fd].get_fd());
}

void WebServer::_read(HttpConn* client) {
    assert(client != nullptr);
    int ret = -1;
    int read_errno = 0;
    ret = client->read(&read_errno);
    if (ret < 0 && read_errno != EAGAIN) {
        _close_conn(client);
        return;
    }
    _process(client);
}

void WebServer::_write(HttpConn* client) {
    assert(client != nullptr);
    int ret = -1;
    int write_errno = 0;
    ret = client->write(&write_errno);
    if (client->write_bytes() == 0) {
        if (client->is_keepalive()) {
            _process(client);
            return;
        }
    }
    else if (ret < 0){
        if (write_errno == EAGAIN) {
            _epoller->mod_fd(client->get_fd(), _conn_event | EPOLLOUT);
            return;
        }
    }
    _close_conn(client);
}

void WebServer::_process(HttpConn* client) {
    if (client->process()) { //http请求处理完毕，读->写/写->读
        _epoller->mod_fd(client->get_fd(), _conn_event | EPOLLOUT);
    }
    else {
        _epoller->mod_fd(client->get_fd(), _conn_event | EPOLLIN);
    }
}