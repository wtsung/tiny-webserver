#include "timer.h"

void time_heap::add_timer(int fd, int timeout, const std::function<void()>& cb) {
    assert(fd > 0);
    size_t i;
    if (_map.count(fd)) {
        i = _heap.size();
        _map[fd] = i;
        struct heap_timer temp;
        temp.fd = fd;
        temp.expire = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);
        temp.cb_func = cb;
        _heap.push_back(temp);
        _siftup(i);
    }
    else {
        i = _map[fd];
        _heap[i].expire = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);
        _heap[i].cb_func = cb;
        if (!_siftdown(i, _heap.size())) {
            _siftup(i);
        }
    }
}

void time_heap::del_timer(size_t index) {
    assert(!_heap.empty() && index >= 0 && index < _heap.size());
    size_t i = index;
    size_t n = _heap.size() - 1;
    assert(i <= n);
    if (i < n) {
        _swap(i, n);
        if (!_siftdown(i, n)) {
            _siftup(i);
        }
    }
    _map.erase(_heap.back().fd);
    _heap.pop_back();
}

void time_heap::adjust_timer(int fd, int timeout) {
    assert(!_heap.empty() && _map.count(fd) > 0);
    _heap[_map[fd]].expire = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);
    _siftdown(_map[fd], _heap.size());
}

void time_heap::pop() {
    assert(!this->empty());
    this->del_timer(0);
}

void time_heap::tick() {
    if (this->empty()) {
        return;
    }
    while (!this->empty()) {
        heap_timer temp = _heap.front();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(temp.expire - std::chrono::system_clock::now()).count() > 0) { 
            break; 
        }
        temp.cb_func();
        this->pop();
    }
}

int time_heap::get_next_tick() {
    tick();
    size_t res = -1;
    if (!_heap.empty()) {
        res = std::chrono::duration_cast<std::chrono::milliseconds>(_heap.front().expire - std::chrono::system_clock::now()).count();
        if (res < 0) {
            res = 0;
        }
    }
    return res;
}

bool time_heap::empty() const {
    return _heap.empty();
}


void time_heap::clear() {
    _heap.clear();
    _map.clear();
}

bool time_heap::_siftdown (size_t idx, size_t n) {
    assert(idx >= 0 && idx < _heap.size());
    assert(n >= 0 && n < _heap.size());
    int i = idx;
    int child = i * 2 + 1;
    while (child < n) {
        if (child + 1 < n && _heap[child + 1] < _heap[child]) {
            child++;
        }
        _swap(i, child);
        i = child;
        child = i * 2 + 1;
    }
    return i > idx;
}

void time_heap::_siftup(size_t idx) {
    assert(idx >= 0 && idx < _heap.size());
    int parent = (idx - 1) / 2;
    while (parent >= 0) {
        if (_heap[parent].expire < _heap[idx].expire) {
            break;
        }
        _swap(parent, idx);
        idx = parent;
        parent = (idx - 1) / 2;
    }
}

void time_heap::_swap (size_t lhs, size_t rhs) {
    assert(lhs >= 0 && lhs < _heap.size());
    assert(rhs >= 0 && rhs < _heap.size());
    std::swap(_heap[lhs],_heap[rhs]);
    _map[_heap[lhs].fd] = lhs;
    _map[_heap[rhs].fd] = rhs;
}