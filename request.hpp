#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;
namespace http=boost::beast::http;
using boost::asio::ip::tcp;


class Request {
public:
    int max_stale;//确定成合适的时间类型，可以做运算的那种
    int min_fresh;//确定成合适的时间类型，可以做运算的那种
    bool has_max_stale;
    bool has_min_fresh;
    Request(http::request<http::string_body> &request) {
        //直接在hpp里面写就行，cpp不用改
        //parse出request里面的max_stale, min_fresh
        //如果存在max_stale, has_max_stale 存成 1; max_stale 存对应的数字
        //如果存在min_fresh, has_min_fresh 存成 1; min_fresh 存对应的数字

        //如果不存在max_stale, has_max_stale 存成 0; max_stale 存成 0 或者其他
        //如果不存在min_fresh, has_min_fresh 存成 0; min_fresh 存成 0 或者其他
    }
};
