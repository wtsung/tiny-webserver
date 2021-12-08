#include "log.h"

Log::Log() {
    _isAsync = false;
    _line_count = 0;
    _write_thread = nullptr;
    _deque = nullptr;
    _fp = nullptr;
}

Log::~Log() {
    if (_write_thread && _write_thread->joinable()) {
        while (!_deque->empty()) {
            _deque->flush();
        }
        _deque->close();
        _write_thread->join();
    }
    if (_fp) {
        std::lock_guard<std::mutex> locker(_mutex);
        flush();
        fclose(_fp);
    }
}

bool Log::init(const char* file_name, int log_buf_size, int split_lines, int max_queue_size) {
       if (max_queue_size > 0) {
           _isAsync = true;//异步
           if (!_deque) {
               _deque = std::make_unique<BlockQueue<std::string>>();
               _write_thread = std::make_unique<std::thread>(flush_log_thread); //异步写线程
           }
       }
       else {
           _isAsync = false;
       }

        _buf_size = log_buf_size;
        _buf = new char[_buf_size];
        memset(_buf, '\0', _buf_size);
        _split_lines = split_lines;
        

       time_t timer = time(nullptr);
       struct tm* systime = localtime(&timer);
       struct tm my_tm = *systime;
       
       const char *p = strrchr(file_name, '/');
       char log_full_name[256] = {0};

       if (p == nullptr) {
           snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
       }
       else {
           strcpy(_log_name, p + 1);
           strncpy(_dir_name, file_name, p - file_name + 1);
           snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", _dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, _log_name);
       }

       _today = my_tm.tm_mday;

        _fp = fopen(log_full_name, "a");
        if (_fp == nullptr) {
            return false;
        }
        return true;
}

void Log::flush_log_thread() {
    Log::get_instance()->async_write();
}
void Log::async_write() {
    std::string single_log;
    while (_deque->pop(single_log)) {
        std::lock_guard<std::mutex> locker(_mutex);
        fputs(single_log.c_str(), _fp);
    }
}

void Log::flush() {
    std::lock_guard<std::mutex> locker(_mutex);
    fflush(_fp);//刷新流 stream 的输出缓冲区。
}

void Log::write_log(int level, const char* format, ...) {
    struct timeval now = {0,0};
    gettimeofday(&now, nullptr);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    std::string s;
    switch (level) {
        case 0:
            s += "[debug]:";
            break;
        case 1:
            s += "[info]:";
            break;
        case 2:
            s += "[warn]:";
            break;
        case 3:
            s += "[error]:";
            break;
        default:
            s += "[info]:";
            break;
    }

    //新建新的日志文件
    if (_today != my_tm.tm_mday || (_line_count && (_line_count % _max_lines == 0))) {
        std::unique_lock<std::mutex> locker(_mutex);
        locker.unlock();

        char new_log[256] = {0};
        char new_tail[16] = {0};

        snprintf(new_tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (_today != my_tm.tm_mday) {
            snprintf(new_log, 255, "%s%s%s", _dir_name, new_tail, _log_name);
            _today = my_tm.tm_mday;
            _line_count = 0;
        }
        else {
            snprintf(new_log, 255, "%s%s%s.%lld", _dir_name, new_tail, _log_name, _line_count / _split_lines);
        }

        locker.lock();
        flush();
        fclose(_fp);
        _fp = fopen(new_log, "a");
        assert(_fp != nullptr);
    }
    
    va_list valst;
    va_start(valst, format);
    std::string log_str;

    {
        std::lock_guard<std::mutex> locker(_mutex);
        _line_count++;
        int n = snprintf(_buf, 48, "%d-%02d-%02d %02d-%02d-%02d.%06ld %s",
        my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_yday, my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_sec, s.c_str());

        int m = vsnprintf(_buf + n, _buf_size - 1, format, valst); //写入可变参数format格式

        _buf[n + m] = '\n';
        _buf[n + m + 1] = '\0';

        log_str = _buf; 
    }
    //异步日志则push到阻塞队列
    if (_isAsync && !_deque->full()) {
        _deque->push_back(log_str);
    }
    else {
        std::lock_guard<std::mutex> locker(_mutex);
        fputs(log_str.c_str(), _fp);
    }
    va_end(valst);
}