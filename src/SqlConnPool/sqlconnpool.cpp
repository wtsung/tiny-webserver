#include "sqlconnpool.h"

SqlConnPool::SqlConnPool() {
    _cur_conn = 0;
    _free_conn = 0;
}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}

void SqlConnPool::ClosePool() {
    std::unique_lock<std::mutex> locker(_mutex);
    while (!_conn_queue.empty()) {
        auto item = _conn_queue.front();
        _conn_queue.pop();
        mysql_close(item);
    }
    _cur_conn = 0;
    _free_conn = 0;
    mysql_library_end();
}

SqlConnPool* SqlConnPool::GetInstance() {
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(std::string url, std::string user, std::string password, std::string databasename, int port,
                       int maxconn, int closelog) {
    assert(maxconn > 0);
    _close_log = closelog;
    _max_conn = maxconn;
    for (int i = 0; i < maxconn; ++i) {
        MYSQL* sql = nullptr;
        sql = mysql_init(sql);
        if (sql == nullptr) {
            if (!_close_log) {
                LOG_ERROR("Mysql init error!");
            }
            assert(sql);
        }
        //登录连接sql
        sql = mysql_real_connect(sql, url.c_str(), user.c_str(), password.c_str(), databasename.c_str(), port, nullptr, 0);

        if (sql == nullptr) {
            if (!_close_log) {
                LOG_ERROR("Mysql connect error!");
            }
            assert(sql);
        }

        //更新连接池
        _conn_queue.push(sql);
        _free_conn++;
    }
    sem_init(&_sem, 0, _max_conn);
}

MYSQL* SqlConnPool::GetConn() {
    MYSQL* sql = nullptr;
    if (_conn_queue.empty()) {
        return nullptr;
    }

    sem_wait(&_sem);
    {
        std::unique_lock<std::mutex> locker(_mutex);
        sql = _conn_queue.front();
        _conn_queue.pop();
    }
    return sql;
}

bool SqlConnPool::ReleaseConn(MYSQL* sql) {
    assert(sql);
    std::unique_lock<std::mutex> locker(_mutex);
    _conn_queue.push(sql);
    sem_post(&_sem);
}

int SqlConnPool::GetFreeConnCount() {
    std::unique_lock<std::mutex> locker(_mutex);
    return _conn_queue.size();
}



