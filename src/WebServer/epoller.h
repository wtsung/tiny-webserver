#ifndef WEBSERVER_EPOLLER_H
#define WEBSERVER_EPOLLER_H

#include <sys/epoll.h>
#include <assert.h>
#include <unistd.h>
#include <vector>
class Epoller {
public:
    explicit Epoller(int max_events = 1024);
    ~Epoller();

    bool add_fd(int fd, uint32_t events);
    bool mod_fd(int fd, uint32_t events);
    bool del_fd(int fd);

    int wait(int timeout_ms = -1);

    int get_event_fd(size_t i) const;
    uint32_t get_events(size_t i) const;

private:
    int _epollfd;
    std::vector<struct epoll_event> _events;
};

#endif WEBSERVER_EPOLLER_H