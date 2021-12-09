#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::_suffix_type {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "}
};

const std::unordered_map<int, std::string> HttpResponse::_code_status { 
    { 200, "OK" }, 
    { 400, "Bad Request" }, 
    { 403, "Forbidden" }, 
    { 404, "Not Found" } 
}; 

const std::unordered_map<int, std::string> HttpResponse::_code_path { 
    { 400, "/400.html" }, 
    { 403, "/403.html" }, 
    { 404, "/404.html" } 
};

HttpResponse::HttpResponse() {
    _code = -1;
    _path = "";
    _root_dir = "";
    _is_keep_alive = false;
    _mmfile = nullptr;
    bzero(&_mmfile_stat, sizeof(_mmfile_stat));
}

HttpResponse::~HttpResponse() {
    this->unmap_file();
}

void HttpResponse::init(const std::string& root_dir, std::string& path, bool is_keey_alive, int code) {
    assert(root_dir != "");
    if (_mmfile) {
        this->unmap_file();
    }
    _code = code;
    _is_keep_alive = is_keey_alive;
    _path = path;
    _root_dir = root_dir;
    _mmfile = nullptr;
    bzero(&_mmfile_stat, sizeof(_mmfile_stat));
}


void HttpResponse::make_response(Buffer& buff) {
    if (stat((_root_dir + _path).data(), &_mmfile_stat) < 0 || S_ISDIR(_mmfile_stat.st_mode)) {
        _code = 404;
    }
    else if (!(_mmfile_stat.st_mode & S_IROTH)) {
        _code = 403; //其他用户没有权限
    }
    else if (_code == -1) {
        _code = 200;
    }
    _error_html();//错误码输出html
    _add_state_line(buff);
    _add_header(buff);
    _add_content(buff);
}

void HttpResponse::_error_html() {
    if (_code_path.count(_code)) {
        _path = _code_path.find(_code)->second;
        stat((_root_dir + _path).data(), &_mmfile_stat);
    }
}


void HttpResponse::unmap_file() {
    if (_mmfile) {
        munmap(_mmfile, _mmfile_stat.st_size);
        _mmfile = nullptr;
    }
}

char* HttpResponse::file() {
    return _mmfile;
}

size_t HttpResponse::file_len() const {
    return _mmfile_stat.st_size;
}

void HttpResponse::error_content(Buffer& buff, std::string message) {
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (_code_status.count(_code)) {
        status = _code_status.find(_code)->second;
    }
    else {
        status = "Bad Request";
    }
    body += std::to_string(_code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}

void HttpResponse::_add_state_line(Buffer& buff) {
    std::string status;
    if (_code_status.count(_code)) {
        status = _code_status.find(_code)->second;
    }
    else {
        _code = 400;
        status = _code_status.find(_code)->second;
    }
    buff.append("HTTP/1.1 " + std::to_string(_code) + " " + status + "\r\n");
}

void HttpResponse::_add_header(Buffer& buff) {
    buff.append("Connection: ");
    if (_is_keep_alive) {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    }
    else {
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + _get_file_type() + "\r\n");
}

void HttpResponse::_add_content(Buffer& buff) {
    int rootfd = open((_root_dir + _path).data(), O_RDONLY);
    if (rootfd < 0) {
        error_content(buff, "File NotFound!");
        return;
    }
    /* 将文件映射到内存提高文件的访问速度 */
    LOG_DEBUG("File path %s", (_root_dir + _path).data());
    int* mmRet = (int*) mmap(0, _mmfile_stat.st_size, PROT_READ, MAP_PRIVATE, rootfd, 0);
    if (*mmRet == -1) {
        error_content(buff, "File NotFound!");
        return;
    }
    _mmfile = (char*)mmRet;
    close(rootfd);
    buff.append("Content-length: " + std::to_string(_mmfile_stat.st_size) + "\r\n\r\n");
}


std::string HttpResponse::_get_file_type() {
    //判断文件类型
    std::string::size_type idx = _path.find_last_of('.');
    //没有找到'.'
    if (idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = _path.substr(idx);
    if (_suffix_type.count(suffix)) {
        return _suffix_type.find(suffix)->second;
    }
    return "text/plain";
}

