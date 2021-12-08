#ifndef WEBSERVER_BLOCKQUEUE_H
#define WEBSERVER_BLOCKQUEUE_H

#include <mutex>
#include <condition_variable>
#include <deque>
#include <cassert>

template <typename T>
class BlockQueue {
public:
    BlockQueue(int max_size = 1000);
    ~BlockQueue();

    void clear();
    bool empty();
    bool full();
    void close();
    int size();
    int capacity();
    T front();
    T back();
    void push_back(const T& item);
    void push_front(const T& item);
    bool pop(T& item);
    bool pop(T& item, int timeout);
    void flush();

private:
    std::mutex _mutex;
    std::condition_variable _cond_consumer;
    std::condition_variable _cond_producer;

    bool _close;
    std::deque<T> _queue;
    int _capacity;
};

template <typename T>
BlockQueue<T>::BlockQueue(int max_size) : _capacity(max_size) {
    assert(max_size > 0);
    _close = false;
}
template <typename T>
BlockQueue<T>::~BlockQueue() {
    close();
}

template <typename T>
void BlockQueue<T>::close() {
    {
        std::unique_lock<std::mutex> locker(_mutex);
        _queue.clear();
        _close = true;
    }
    _cond_producer.notify_all();
    _cond_consumer.notify_all();
}

template <typename T>
void BlockQueue<T>::flush() {
    _cond_consumer.notify_one();
}

template <typename T>
void BlockQueue<T>::clear() {
    std::unique_lock<std::mutex> locker(_mutex);
    _queue.clear();
}

template <typename T>
T BlockQueue<T>::front() {
    std::unique_lock<std::mutex> locker(_mutex);
    return _queue.front();
}

template <typename T>
T BlockQueue<T>::back() {
  std::lock_guard<std::mutex> locker(_mutex);
  return _queue.back();
}

template <typename T>
int BlockQueue<T>::size() {
  std::lock_guard<std::mutex> locker(_mutex);
  return _queue.size();
}

template <typename T>
int BlockQueue<T>::capacity() {
  std::lock_guard<std::mutex> locker(_mutex);
  return _capacity;
}

template <typename T>
void BlockQueue<T>::push_back(const T& item) {
  std::unique_lock<std::mutex> locker(_mutex);
  if (_queue.size() >= _capacity) {
    _cond_producer.wait(locker); //使用locker锁住当前线程，直到被notify唤醒
  }
  _queue.push_back(item);
  _cond_consumer.notify_one(); //通知消费者条件变量
}

template <typename T>
void BlockQueue<T>::push_front(const T& item) {
  std::unique_lock<std::mutex> locker(_mutex);
  if  (_queue.size() >= capacity()) {
    _cond_producer.wait(locker);
  }
  _queue.push_front(item);
  _cond_consumer.notify_one();
}

template <typename T>
bool BlockQueue<T>::empty() {
  std::lock_guard<std::mutex> locker(_mutex);
  return _queue.empty();
}

template <typename T>
bool BlockQueue<T>::full() {
  std::lock_guard<std::mutex> locker(_mutex);
  return _queue.size() >= _capacity;
}

template <typename T>
bool BlockQueue<T>::pop(T &item) {
  std::unique_lock<std::mutex> locker(_mutex);
  while (_queue.empty()) {
    _cond_consumer.wait(locker);
    if (_close) {
      return false;
    }
  }
  item = _queue.front();
  _queue.pop_front();
  _cond_producer.notify_one();
  return true;
}

template <typename T>
bool BlockQueue<T>::pop(T& item, int timeout) {
  std::unique_lock<std::mutex> locker(_mutex);
  while (_queue.empty()) {
    if (_cond_consumer.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout) {
      return false;
    }
    if (_close) {
      return false;
    }
  }
  item = _queue.front();
  _queue.pop_front();
  _cond_producer.notify_one();
  return true;
}



#endif //WEBSERVER_BLOCKQUEUE_H
