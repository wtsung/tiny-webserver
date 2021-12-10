#ifndef WEBSERVER_HTTPCONN_H
#define WEBSERVER_HTTPCONN_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <unistd.h>
#include "httprequest.h"
#include "httpresponse.h"
#include "../Log/log.h"
#include "../Buffer/buffer.h"
class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    void init(int sockfd, const sockaddr_in& addr);
    void close_conn();

    int get_fd() const;
    int get_port() const;
    char* get_ip() const;
    sockaddr_in get_addr() const;

    ssize_t read(int* save_errno);
    ssize_t write(int* save_errno);

    bool process();

    int write_bytes();
    
    bool is_keepalive() const;

    static bool _trig_ET;
    static std::string _root_dir;
    static std::atomic<int> _user_count;
    
private:
    
    int _sockfd;
    struct sockaddr_in _addr;

    bool _close;

    int _iov_count;
    struct iovec _iov[2];

    std::unique_ptr<Buffer> _read_buff;
    std::unique_ptr<Buffer> _write_buff;



    std::unique_ptr<HttpRequest> _request;
    std::unique_ptr<HttpResponse> _response;
};


#endif //WEBSERVER_HTTPCONN_H