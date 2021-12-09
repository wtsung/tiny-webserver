#include "httpconn.h"

HttpConn::HttpConn() {
    _sockfd = -1;
    bzero(&_addr, sizeof(_addr));
    _close = true;
}

HttpConn::~HttpConn() {
    close_conn();
}

void HttpConn::init(int sockfd, const sockaddr_in& addr) {
    assert(sockfd > 0);
    _user_count++;
    _addr = addr;
    _sockfd = sockfd;
    _read_buff->retrieve_all();
    _write_buff->retrieve_all();
    _close = false;
    LOG_INFO("Client[%d](%s:%d) in, UserCount: %d", _sockfd, this->get_ip(), this->get_port(), (int)_user_count);
}

void HttpConn::close_conn() {
    _response->unmap_file();
    if (_close == false) {
        _close = true;
        _user_count--;
        close(_sockfd);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", _sockfd, get_ip(), get_port(), (int)_user_count);
    }
}

int HttpConn::get_fd() const {
    return _sockfd;
}

sockaddr_in HttpConn::get_addr() const {
    return _addr;
}

char* HttpConn::get_ip() const {
    return inet_ntoa(_addr.sin_addr);//输出点分十进制ip字符串
}

int HttpConn::get_port() const {
    return _addr.sin_port;
}

ssize_t HttpConn::read(int* save_errno) {
    ssize_t len = -1;
    do {
        len = _read_buff->read_fd(_sockfd, save_errno);
        if (len < 0) {
            break;
        }
    } while (_trig_ET);
    return len;
}

ssize_t HttpConn::write(int* save_errno) {
    ssize_t len = -1;
    do {
        len = writev(_sockfd, _iov, _iov_count);
        if (len <= 0) {
            *save_errno = errno;
            break;
        }
        if (_iov[0].iov_len + _iov[1].iov_len == 0) {
            break;
        }
        //更新iov1的base和len
        else if (static_cast<size_t>(len) > _iov[0].iov_len) {
            _iov[1].iov_base = (uint8_t*) _iov[1].iov_base + (len - _iov[0].iov_len);
            _iov[1].iov_len -= (len - _iov[0].iov_len);
            if (_iov[0].iov_len) {
                _write_buff->retrieve_all();
                _iov[0].iov_len = 0;
            }
        }
        else {
            _iov[0].iov_base = (uint8_t*)_iov[0].iov_base + len;
            _iov[0].iov_len -= len;
            _write_buff->retrieve(len);
        }
    } while (_trig_ET || this->write_bytes() > 10240);
    return len;
}

bool HttpConn::process() {
    _request->init();
    if (_read_buff->readable_bytes() <= 0) {
        return false;
    }
    else if (_request->parse(*_read_buff.get())) {
        LOG_DEBUG("%s", _request->path().c_str());
        _response->init(_root_dir, _request->path(), this->is_keepalive(), 200);
    }
    else {
        _response->init(_root_dir, _request->path(), false, 400);
    }
    _response->make_response(*_write_buff.get());
    //响应头，在buff中
    _iov[0].iov_base = const_cast<char*> (_write_buff->peek());
    _iov[0].iov_len = _write_buff->readable_bytes();
    _iov_count = 1;

    //文件
    if (_response->file_len() > 0 && _response->file()) {
        _iov[1].iov_base = _response->file();
        _iov[1].iov_len = _response->file_len();
        _iov_count = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", _response->file_len() , _iov_count, write_bytes());////
    return true;
}

int HttpConn::write_bytes() {
    return _iov[0].iov_len + _iov[1].iov_len;
}
    
bool HttpConn::is_keepalive() const {
    return _request->is_keepAlive();
}

