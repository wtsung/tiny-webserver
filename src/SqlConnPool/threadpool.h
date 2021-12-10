#ifndef WEBSERVER_THREADPOOL_H
#define WEBSERVER_THREADPOOL_H

#include <mutex>
#include <thread>
#include <queue>
#include <vector>
#include <condition_variable>
#include <functional>
#include <memory>
#include <assert.h>

class ThreadPool {
public:
    ThreadPool(int thread_number = 8) : _thread_number(thread_number) {
       assert(thread_number > 0); 
        _close = false;
        for (int i = 0; i < thread_number; ++i) 
        {
            _thread_array.emplace_back(
                [this]
                {
                    for (;;) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> locker(this->_mutex);
                            this->_cond.wait(locker, [this]{ return this->_close || !this->_task_queue.empty(); });
                            if (this->_close && this->_task_queue.empty()) {
                                return;
                            }
                            task = std::move(this->_task_queue.front());
                            this->_task_queue.pop();
                        }
                        task();
                    }
                }
            ); 
        } 
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> locker(_mutex);
            _close = true;
        }
        _cond.notify_all();
        //回收线程
        for (auto& thread : _thread_array) {
            thread.join();
        }
    }
    template <typename T>
    void AddTask(T&& task) {
        {
            std::unique_lock<std::mutex> locker(_mutex);
            _task_queue.push(std::forward<T>(task));
        }
        _cond.notify_one();
    }
private:
    int _thread_number; //线程数
    std::queue< std::function<void()> > _task_queue; //任务队列
    std::vector< std::thread > _thread_array; //线程数组
    std::mutex _mutex;
    std::condition_variable _cond;
    bool _close;

};

#endif //WEBSERVER_THREADPOOL_H
