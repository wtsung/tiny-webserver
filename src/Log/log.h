#ifndef WEBSERVER_LOG_H
#define WEBSERVER_LOG_H

#include "blockqueue.h"
#include <string>
#include <cstring>
#include <thread>
#include <stdarg.h> 
#include <mutex>
#include <memory>
#include <ctime>
#include <cassert>
#include <sys/time.h>

class Log {
public:
    bool init(const char* file_name, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);
    static Log* get_instance() {
        static Log instance;
        return &instance;
    }
    void flush_log_thread();

    void write_log(int level, const char* format, ...);
    
    void flush();

private:
    Log();
    ~Log();

    void async_write();

    static const int MAX_LINES = 50000;

    int _max_lines;
    int _line_count;
    bool _isAsync; //是否异步
    int _level;
    int _split_lines;
    int _buf_size;
    int _today;
    char* _buf;
    
    char _log_name[128];
    char _dir_name[128];

    FILE* _fp;
    std::unique_ptr<BlockQueue<std::string>> _deque;
    std::unique_ptr<std::thread> _write_thread;
    std::mutex _mutex;
};

#define LOG_DEBUG(format, ...) do {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();} while (0);
#define LOG_INFO(format, ...) do {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();} while (0);
#define LOG_WARN(format, ...) do {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();} while (0);
#define LOG_ERROR(format, ...)  do {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();} while (0);



#endif //WEBSERVER_LOG_H