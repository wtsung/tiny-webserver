#ifndef WEBSERVER_TIMER_H
#define WEBSERVER_TIMER_H

#include <netinet/in.h>
#include <memory>
#include <functional>
#include <time.h>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <cassert>
#define BUFFER_SIZE 64

//定时器类
struct heap_timer {
public:
    int fd;
    std::chrono::system_clock::time_point expire; //定时器生效的时间
    std::function<void()> cb_func;
    bool operator < (const heap_timer& t) {
        return expire < t.expire;
    }
};

//时间堆
class time_heap {
public:
    time_heap() = default;
    ~time_heap() = default;
    void add_timer(int fd, int timeout, const std::function<void()>& cb);
    void del_timer(size_t i);
    void adjust_timer(int fd, int timeout);
    void pop();
    void tick(); //清除超时定时器
    int get_next_tick(); 
    void clear();
    bool empty() const;
private:
    void _siftup(size_t i);
    bool _siftdown(size_t idx, size_t n);
    void _swap(size_t lhs, size_t rhs);
private:
    std::vector<heap_timer> _heap;
    std::unordered_map<int, size_t> _map;
};




#endif //WEBSERVER_TIMER_H