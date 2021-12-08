#ifndef WEBSERVER_HTTPREQUEST_H
#define WEBSERVER_HTTPREQUEST_H

#include <string>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <mysql/mysql.h>

#include "../Buffer/buffer.h"
#include "../SqlConnPool/sqlconnpoolRAII.h"
#include "../Log/log.h"
class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADER,
        CONTENT,
        FINISH
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAN_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSE_CONNECTION
    };

    HttpRequest() { init(); }
    ~HttpRequest() = default;

    void init();
    bool parse(Buffer& buff);

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string get_post(const std::string& key) const;
    std::string get_post(const char* key) const;

    bool is_keepAlive() const;
    
private:
    bool _parse_requestline(const std::string& line);
    void _parse_header(const std::string& line);
    void _parse_content(const std::string& line);

    void _parse_path();
    void _parse_post();
    void _parse_from_urlencoded();

    static bool user_verify(const std::string& name, const std::string& pwd, bool is_login);

    PARSE_STATE _state;
    std::string _method;
    std::string _path;
    std::string _version;
    std::string _content;
    std::unordered_map<std::string, std::string> _header; //存储请求头内容
    std::unordered_map<std::string, std::string> _post;   //存储请求体内容

    static const std::unordered_set<std::string> _default_html;
    static const std::unordered_map<std::string, int> _default_html_tag;
    static int _cover_hex(char ch);
};

#endif //WEBSERVER_HTTPREQUEST_H
