#include "httprequest.h"

const std::unordered_set<std::string> HttpRequest::_default_html {"/index", "/register", "/login", "/welcome", "/vedio", "/picture"};

const std::unordered_map<std::string, int> HttpRequest::_default_html_tag { {"/register.html", 0}, {"/login.html", 1} };

void HttpRequest::init() {
    _method = "";
    _path = "";
    _version = "";
    _content = "";
    _state = REQUEST_LINE;
    _header.clear();
    _post.clear();
}

bool HttpRequest::is_keepAlive() const {
    if (_header.count("Connection") == 1) {
        return _header.find("Connection")->second == "keep-alive" && _version == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    //判断是否有可读内容
    if (buff.readable_bytes() <= 0) {
        return false;
    }
    //状态机
    while (buff.readable_bytes() && _state != FINISH) {
        const char* line_end = std::search(buff.peek(), buff.begin_write_const(), CRLF, CRLF + 2);//待读取-已写；找到CRLF在buff中的位置，返回buff中的位置
        std::string line(buff.peek(), line_end);
        switch (_state)
        {
        case REQUEST_LINE:
            //处理请求行
            if (!_parse_requestline(line)) {
                return false;
            }
            //处理请求url
            _parse_path();
            break;
        case HEADER:
            //处理请求头
            _parse_header(line);
            //buff读取完毕，无content
            if (buff.readable_bytes() <= 2) {
                _state = FINISH;
            }
            break;
        case CONTENT:
            //处理请求体
            _parse_content(line);
            break;
        default:
            break;
        }
        //读取到已写部分
        if (line_end == buff.begin_write()) {
            break;
        }
        //更新_read_pos
        buff.retrieve_until(line_end + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", _method.c_str(), _path.c_str(), _version.c_str());
    return true;
}

bool HttpRequest::_parse_requestline(const std::string& line) {
    //^ 当前为主为文本开始位置， $文本末尾
    //[^ ]* 表示匹配0或多个非空格
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    //smatch容器类，保存在目标串中搜索的结果
    std::smatch submatch;
    //将目标串和正则表达式进行匹配，两者必须完全匹配，
    if (std::regex_match(line, submatch, patten)) {
        _method = submatch[1];
        _path = submatch[2];
        _version = submatch[3];
        _state = HEADER;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::_parse_header(const std::string& line) {
    //([^:]*)表示匹配0或多个非冒号
    //？表示匹配0或1次
    //(.*)表示匹配0或多个除了换行符（\n）以外的任意一个字符
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch submatch;
    //匹配成功，否则header下有空行，进入_state为CONTENT
    if (std::regex_match(line, submatch, patten)) {
        _header[submatch[1]] = submatch[2]; //存入map
    }
    else {
        _state = CONTENT;
    }

}
void HttpRequest::_parse_content(const std::string& line) {
    _content = line;
    //处理post请求的请求体，存储登录信息
    _parse_post();
    _state = FINISH;
    LOG_DEBUG("Content:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::_parse_path() {
    if (_path == "/") {
        _path = "/index.html";
    }
    else {
        for (auto& item : _default_html) {
            if (item == _path) {
                _path += ".html";
                break;
            }
        }
    }
}

void HttpRequest::_parse_post() {
    //content编码格式为"application/x-www-form-urlencoded"，例如name1=value1&name2=value2…
    if (_method == "POST" && _header["Content-Type"] == "application/x-www-form-urlencoded") {
        //解析存入post的map中
        _parse_from_urlencoded();
        //登录or注册
        if (_default_html_tag.count(_path)) {
            int tag = _default_html_tag.find(_path)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (user_verify(_post["username"], _post["password"], isLogin)) {
                    _path = "/welcome.html";
                }
                else {
                    _path = "/error.html";
                }
            }
        }
    }
}

//十六进制处理
int HttpRequest::_cover_hex (char ch) {
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return ch;
}

//处理post请求体内容
void HttpRequest::_parse_from_urlencoded() {
    if (_content.size() == 0) {
        return;
    }
    std::string key;
    std::string value;
    int num = 0;
    int n = _content.size();
    int i = 0;
    int j = 0;
    //content编码格式为"application/x-www-form-urlencoded"
    //键对=，空格+，间隔&，两位十六进制数%
    for (; i < n; ++i) {
        char ch = _content[i];
        switch (ch)
        {
        case '=':
            key = _content.substr(j, i - j);
            j = i + 1;    
            break;
        case '+':
            _content[i] = ' ';
            break;
        case '%':
            num = _cover_hex(_content[i + 1]) * 16 + _cover_hex(_content[i + 2]);
            _content[i + 2] = num % 10 + '0';
            _content[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = _content.substr(j, i - j);
            j = i + 1;
            _post[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;                   
        default:
            break;
        }
    }
    assert(j <= i);
    //最后一个未存入
    if (_post.count(key) == 0 && j < i) {
        value = _content.substr(j, i - j);
        _post[key] = value;
    }
}

bool HttpRequest::user_verify(const std::string& name, const std::string& pwd, bool is_login) {
    if (name == "" || pwd == "") {
        return false;
    }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnPoolRAII sqlcon(&sql, SqlConnPool::GetInstance());
    assert(sql);

    bool flag = 0;
    unsigned int j = 0;
    char order[256] = {0};
    MYSQL_FIELD* fields = nullptr;
    MYSQL_RES* res = nullptr;

    if (!is_login) {
        flag = true;
    }
    //用户查询 
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if (mysql_query(sql, order)) {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        //若登录，则匹配密码；否则注册，用户名被占用
        if(is_login) {
            if (pwd == password) {
                flag = true;
            }
            else {
                flag = false;
                LOG_DEBUG("password error!");
            }
        }
        else {
            flag = false;
            LOG_DEBUG("username used!");
        }
    }
    mysql_free_result(res);

    //注册，且用户名可用
    if (!is_login && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s', '%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order)) {
            LOG_DEBUG("Insert error!");
            flag = false;
        }
        flag = true;
    }
    LOG_DEBUG("UserVerify success!!");
    return flag;

}



std::string HttpRequest::path() const {
    return _path;
}
std::string& HttpRequest::path() {
    return _path;
}
std::string HttpRequest::method() const {
    return _method;
}
std::string HttpRequest::version() const {
    return _version;
}
std::string HttpRequest::get_post(const std::string& key) const {
    assert(key != "");
    if (_post.count(key)) {
        return _post.find(key)->second;
    }
    return "";
}
std::string HttpRequest::get_post(const char* key) const {
    assert(key != "");
    if (_post.count(key)) {
        return _post.find(key)->second;
    }
    return "";
}
