#ifndef WEBSERVER_BUFFER_H
#define WEBSERVER_BUFFER_H
#include <string>
#include <vector>
#include <atomic>
#include <assert.h>
#include <unistd.h>
#include <sys/uio.h>

class Buffer {
public:
    Buffer(int size = 1024);
    ~Buffer() = default;

    size_t writeable_bytes() const;   //可写空间
    size_t readable_bytes() const;    //可读空间
    size_t prependable_bytes() const; //已读空间

    const char* peek() const;          //当前待读位置
    void ensure_writeable(size_t len); //空间不够则拓展，确保可写
    void has_written(size_t len);      //更新已写位置_write_pos

    void retrieve(size_t len);            //更新已读位置_read_pos
    void retrieve_until(const char* end); //更新已读位置_read_pos到指定字符
    void retrieve_all();                  //释放/取回所有空间
    std::string retrieve_alltostr();      //读出未读的str后，释放/取回所有空间

    const char* begin_write_const() const;
    char* begin_write();

    void append(const std::string& str);
    void append(const char* str, size_t len);
    void append(const void* data, size_t len);
    void append(const Buffer& buff);

    ssize_t read_fd(int fd, int* save_errno); //fd读出，写入buffer
    ssize_t write_fd(int fd, int* save_errno); //buffer读出，写入fd

private:
    char* _begin_ptr();
    const char* _begin_ptr() const;
    void _make_space(size_t len);

    std::vector<char> _buffer;
    //std::atomic对int, char, bool等数据结构进行原子性封装，在多线程环境中，对std::atomic对象的访问不会造成竞争-冒险。实现数据结构的无锁设计。
    std::atomic<size_t> _read_pos;
    std::atomic<size_t> _write_pos;
};

#endif //WEBSERVER_BUFFER_H