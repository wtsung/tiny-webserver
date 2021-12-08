#ifndef WEBSERVER_SQLCONNPOOLRAII_H
#define WEBSERVER_SQLCONNPOOLRAII_H

#include "sqlconnpool.h"

/* 资源在对象构造初始化 资源在对象析构时释放*/
class SqlConnPoolRAII {
public:
    SqlConnPoolRAII(MYSQL** sql, SqlConnPool* pool);
    ~SqlConnPoolRAII();
private:
    MYSQL* _sqlRAII;
    SqlConnPool* _poolRAII;
};

SqlConnPoolRAII::SqlConnPoolRAII(MYSQL** sql, SqlConnPool* pool) {
    assert(pool);
    *sql = pool->GetConn();
    _sqlRAII = *sql;
    _poolRAII = pool;
}

SqlConnPoolRAII::~SqlConnPoolRAII() {
    _poolRAII->ReleaseConn(_sqlRAII);
}

#endif //WEBSERVER_SQLCONNPOOLRAII_H
