#ifndef WEBSERVER_HTTPRESPONSE_H
#define WEBSERVER_HTTPRESPONSE_H

#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unordered_map>
#include "../Buffer/buffer.h"
#include "../Log/log.h"

class HttpResponse {
    HttpResponse();
    ~HttpResponse();

    void init(const std::string& root_dir, std::string& path, bool is_keey_alive = false, int code = -1);

    void make_response(Buffer& buff);
    void unmap_file();
    char* file();
    size_t file_len() const;
    void error_content(Buffer& buff, std::string message);
    int code() const { return _code; }

private:
    void _add_state_line(Buffer& buff);
    void _add_header(Buffer& buff); 
    void _add_content(Buffer& buff);

    void _error_html();
    std::string _get_file_type();

    int _code;
    bool _is_keep_alive;

    std::string _path;
    std::string _root_dir;

    char* _mmfile;
    struct stat _mmfile_stat;

    static const std::unordered_map<std::string, std::string> _suffix_type;
    static const std::unordered_map<int, std::string> _code_status;
    static const std::unordered_map<int, std::string> _code_path;
};

#endif //WEBSERVER_HTTPRESPONSE_H