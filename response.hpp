#include <boost/beast/http.hpp>
#include <iostream>
#include <vector>
namespace http=boost::beast::http;

class Response {
private:
    http::response<http::dynamic_body> response;
    std::vector<std::pair<boost::beast::string_view, boost::beast::string_view>> headers;
public:
    Response(http::response<http::dynamic_body> res):response(res){
        parseHeader();
    }
    void parseHeader();
    bool noCache();
};