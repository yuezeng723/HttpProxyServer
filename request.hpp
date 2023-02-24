#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;
class Request {
private:
    string method;
    string url;
public:
    Request(string method, string url): method(method), url(url) {}
    string getMethod() {return method;}
    string getUrl() {return url; }
};
