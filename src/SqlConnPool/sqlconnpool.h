#ifndef WEBSERVER_SQLCONNPOOL_H
#define WEBSERVER_SQLCONNPOOL_H

#include <mutex>
#include <queue>
#include <mysql/mysql.h>
#include <semaphore.h>
#include <string>
#include <cassert> 
#include "../Log/log.h"
class SqlConnPool {
public:
    MYSQL* GetConn();
    bool ReleaseConn(MYSQL* conn);
    int GetFreeConnCount();

    static SqlConnPool* GetInstance();

    void Init(std::string url, std::string user, std::string password, std::string databasename, int port, int maxconn, int closelog = 0);
    void ClosePool();
private:
    SqlConnPool();
    ~SqlConnPool();

    int _max_conn;
    int _cur_conn;
    int _free_conn;
    std::mutex _mutex;
    std::queue<MYSQL*> _conn_queue;
    sem_t _sem;

    int _close_log;
};

#endif //WEBSERVER_SQLCONNPOOL_H
