#include "buffer.h"

Buffer::Buffer(int size) : _buffer(size), _read_pos(0), _write_pos(0) {}

size_t Buffer::writeable_bytes() const {
    return _buffer.size() - _write_pos;
}

size_t Buffer::readable_bytes() const {
    return _write_pos - _read_pos;
}

size_t Buffer::prependable_bytes() const {
    return _read_pos;
}

const char* Buffer::peek() const {
    return _begin_ptr() + _read_pos;
}

void Buffer::ensure_writeable(size_t len) {
    if (this->writeable_bytes() < len) {
        _make_space(len);
    }
    assert(this->writeable_bytes() >= len);
}

void Buffer::has_written(size_t len) {
    _write_pos += len;
}

void Buffer::retrieve(size_t len) {
    assert(len < readable_bytes());
    _read_pos += len;
}

void Buffer::retrieve_until(const char* end) {
    assert(peek() <= end);
    retrieve(end - peek());
}

void Buffer::retrieve_all() {
    bzero(&_buffer[0], _buffer.size());
    _read_pos = 0;
    _write_pos = 0;
}

std::string Buffer::retrieve_alltostr() {
    std::string str(this->peek(), this->readable_bytes()); //读出未读的str
    this->retrieve_all();                                  //释放/取回所有空间
    return str; 
}

const char* Buffer::begin_write_const() const {
    return this->_begin_ptr() + _write_pos;
}

char* Buffer::begin_write() {
    return _begin_ptr() + _write_pos;
}

void Buffer::append(const std::string& str) {
    this->append(str.data(), str.length());
}

void Buffer::append(const char* str, size_t len) {
    assert(str);
    this->ensure_writeable(len);
    std::copy(str, str + len, begin_write());//str-str+len拷贝到begin_write()处
    this->has_written(len);
}

void Buffer::append(const void* data, size_t len) {
    assert(data);
    this->append(static_cast<const char*>(data), len);
}

void Buffer::append(const Buffer& buff) {
    this->append(buff.peek(), buff.readable_bytes());
}

//fd读出，写入buffer
ssize_t Buffer::read_fd(int fd, int* save_errno) {
    char buff[65535];
    struct iovec iov[2]; //struct iovec存储地址和长度，以进行io
    const size_t writeable = this->writeable_bytes(); //可写size

    //分散读
    iov[0].iov_base = this->_begin_ptr() + _write_pos;
    iov[0].iov_len = writeable; //先写入当前buffer剩余可写空间
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff); //写入临时buff

    const ssize_t len = readv(fd, iov, 2); //从fd读
    if (len < 0) {
        *save_errno = errno;
    }
    else if (static_cast<size_t>(len) <= writeable) {
        _write_pos += len;
    }
    else {
        _write_pos = _buffer.size();
        this->append(buff, len - writeable);
    }
    return len;
}

ssize_t Buffer::write_fd(int fd, int* save_errno) {
    size_t readsize = this->readable_bytes();  //待读取数
    ssize_t len = write(fd, this->peek(), readsize);
    if (len < 0) {
        *save_errno = errno;
        return len;
    }
    _read_pos += len;
    return len;
}

char* Buffer::_begin_ptr() {
    return &*_buffer.begin();
}
const char* Buffer::_begin_ptr() const {
    return &*_buffer.begin();
}
void Buffer::_make_space(size_t len) {
    if (this->writeable_bytes() + this->prependable_bytes() < len) {
        _buffer.resize(_write_pos + len + 1);
    }
    else {
        size_t readable = this->readable_bytes();//待读取
        std::copy(this->_begin_ptr() + _read_pos, this->_begin_ptr() + _write_pos, this->_begin_ptr());
        _read_pos = 0;
        _write_pos = _read_pos + readable;
        assert(readable == this->readable_bytes());
    }
}