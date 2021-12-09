#ifndef WEBSERVER_SQLCONNPOOLRAII_H
#define WEBSERVER_SQLCONNPOOLRAII_H

#include "sqlconnpool.h"

/* 资源在对象构造初始化 资源在对象析构时释放*/
class SqlConnPoolRAII {
public:
    SqlConnPoolRAII(MYSQL** sql, SqlConnPool* pool) {
        assert(pool);
        *sql = pool->GetConn();
        _sqlRAII = *sql;
        _poolRAII = pool;
    }
    ~SqlConnPoolRAII()  {
        if (_sqlRAII) {
            _poolRAII->ReleaseConn(_sqlRAII);
        }
    }
private:
    MYSQL* _sqlRAII;
    SqlConnPool* _poolRAII;
};


#endif //WEBSERVER_SQLCONNPOOLRAII_H
