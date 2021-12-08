#ifndef WEBSERVER_TIMER_H
#define WEBSERVER_TIMER_H

#include <netinet/in.h>
#include <memory>
#include <functional>
#include <time.h>
#include <vector>
#include <unordered_map>
#include <chrono>
#define BUFFER_SIZE 64

class heap_timer;
//绑定socket/address和定时器
struct client_data {
    struct sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    heap_timer* timer;
};

//定时器类
class heap_timer {
public:
    heap_timer() {
        expire = std::chrono::system_clock::now();
    }
    heap_timer(int delay) {
        expire = std::chrono::system_clock::now() + std::chrono::milliseconds(delay);
    }
    std::chrono::system_clock::time_point expire; //定时器生效的时间
    std::function<void()> cb_func;
    client_data* user_data;
};

//时间堆
class time_heap {
public:
    time_heap() = default;
    ~time_heap() = default;
    void add_timer(heap_timer* timer);
    void del_timer(heap_timer* timer);
    void adjust_timer(heap_timer* timer, int timeout);
    heap_timer* top() const;
    void pop();
    void tick(); //清除超时定时器
    int get_next_tick(); 
    void clear();
    bool empty() const;
private:
    void _siftup(int hole);
    void _siftdown(int hole);
    void _swap(int lhs, int rhs);
private:
    std::vector<std::shared_ptr<heap_timer> > _heap;
    std::unordered_map<heap_timer*, int> _map;
};




#endif //WEBSERVER_TIMER_H