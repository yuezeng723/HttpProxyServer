#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <vector>
using namespace std;
namespace http=boost::beast::http;
using boost::asio::ip::tcp;
class Response {
private:
    http::response<http::dynamic_body> response;
    std::string cache_control_string;
    std::string etag_string;
    std::string last_modify_string;
public:
    int status_code; 
    bool noCache;//1 有 存，但要revalidation
    bool noStore;//1 有 如果可以直接判断 cache_control_string 是空的，那么就把noStore存成1
    bool pri;//private 
    bool mustRevalidate;
    int max_age;
    int max_stale;//删除这个字段
    Response():noCache(0),noStore(0),pri(0),mustRevalidate(0),max_age(0),max_stale(0){}
    Response(http::response<http::dynamic_body> res):response(res){
        parseHeader();
        status_code = response.result_int();
    }
    void parseHeader();
    void searchCacheControl();
    std::string getETAG(){return etag_string;}
    std::string getLastModify(){return last_modify_string;}
    int getStatusCode(){return status_code;}
};