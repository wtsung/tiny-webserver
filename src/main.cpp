#include "WebServer/webserver.h"

int main() {
    WebServer server(1316, 3, 60000, false, //端口，ET，timeout，优雅关闭
                     "root", "123456", "yourdb", 3306,  //sql
                     false, 12, 6, 1024); //日志, sql连接数，线程数，日志队列长度
    server.start();
    return 0;
}
