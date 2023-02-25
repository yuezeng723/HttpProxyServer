#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;
class Request {
private:
    string method;
    string url;
    string hostname;
    string port;
public:
    Request(string method, string url): method(method), url(url) {}
    string getMethod() {return method;}
    string getUrl() {return url; }
    string getHostname() {return hostname;}
    string getPort() {return port;}
    void sepHostPort();
};
