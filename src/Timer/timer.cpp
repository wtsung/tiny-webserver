#include "timer.h"

void time_heap::add_timer(heap_timer* timer) {
    if (timer == nullptr) {
        return;
    }
    std::shared_ptr<heap_timer> sp_timer;
    sp_timer.reset(timer);
    _heap.push_back(sp_timer);
    _siftup(_heap.size() - 1);
}

void time_heap::del_timer(heap_timer* timer) {
    assert(!_heap.empty() && timer != nullptr);
    timer->cb_func = nullptr;
    _map.erase(timer);
}

void time_heap::adjust_timer(heap_timer* timer, int timeout) {
    assert(!_heap.empty() && _map.count(timer) > 0);
    int idx = _map[timer];
    _heap[_map[timer]]->expire = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);
    _siftdown(_map[timer]);
}

heap_timer* time_heap::top() const {
    if (this->empty()) {
        return nullptr;
    }
    return _heap[0].get();
}

void time_heap::pop() {
    if (this->empty()) {
        return;
    }
    if (_heap[0].get()) {
        _map.erase(_heap[0].get());
        _heap[0] = _heap.back();
        _map[_heap[0].get()] = 0;
        _heap.pop_back();
        _siftdown(0);
    }
}

void time_heap::tick() {
    assert(!this->empty());
    std::unique_ptr<heap_timer> tmp;
    auto cur = std::chrono::system_clock::now();
    tmp.reset(this->top());
    while (!this->empty()) {
        if (tmp->expire > cur) {
            break;
        }
        if (tmp->cb_func) {
            tmp->cb_func();
        }
        this->pop();
        tmp.reset(this->top());
    }
}

int time_heap::get_next_tick() {
    tick();
    size_t res = -1;
    if (!_heap.empty()) {
        res = std::chrono::duration_cast<std::chrono::milliseconds>(_heap.front()->expire - std::chrono::system_clock::now()).count();
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

void time_heap::_siftdown (int hole) {
    assert(hole >= 0 && hole < _heap.size());
    std::unique_ptr<heap_timer> temp;
    temp.reset(_heap[hole].get());
    int idx = hole;
    int child = 0;
    for (;(idx * 2 + 1) <= _heap.size() - 1; idx = child) {
        child = idx * 2 + 1;
        if ((child < (_heap.size() - 1)) && (_heap[child + 1]->expire < _heap[child]->expire)) {
            child++;
        }
        if (_heap[child]->expire < _heap[idx]->expire) {
            _heap[idx] = _heap[child];
            _map[_heap[child].get()] = idx;
        }
        else {
            break;
        }
    }
    _heap[idx].reset(temp.get());
    _map[temp.get()] = idx;
}

void time_heap::_siftup(int hole) {
    assert(hole >= 0 && hole < _heap.size());
    int idx = hole;
    int parent = (idx - 1) / 2;
    while (parent >= 0) {
        if (_heap[parent]->expire < _heap[idx]->expire) {
            break;
        }
        _swap(parent, idx);
        idx = parent;
        parent = (idx - 1) / 2;
    }
}

void time_heap::_swap (int lhs, int rhs) {
    assert(lhs >= 0 && lhs < _heap.size());
    assert(rhs >= 0 && rhs < _heap.size());
    std::swap(_heap[lhs],_heap[rhs]);
    _map[_heap[lhs].get()] = lhs;
    _map[_heap[rhs].get()] = rhs;
}