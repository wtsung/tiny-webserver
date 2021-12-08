#include "epoller.h"

Epoller::Epoller(int max_event) : _epollfd(epoll_create(512)), _events(max_event) {
    assert(_epollfd > 0 && _events.size() > 0);
}

Epoller::~Epoller() {
    close(_epollfd);
}

bool Epoller::add_fd(int fd, uint32_t events) {
    if (fd < 0) {
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(_epollfd, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::mod_fd(int fd, uint32_t events) {
    if (fd < 0) {
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::del_fd(int fd) {
    if (fd < 0) {
        return false;
    }
    epoll_event ev = {0};
    return 0 == epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::wait(int timeout_ms) {
    return epoll_wait(_epollfd, &_events[0], static_cast<int>(_events.size()), timeout_ms);
}

int Epoller::get_event_fd(size_t i) const {
    assert(i < _events.size() && i>= 0);
    return _events[i].data.fd;
}

uint32_t Epoller::get_events(size_t i) const {
    assert(i < _events.size() && i >= 0);
    return _events[i].events;
}