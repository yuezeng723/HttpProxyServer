#include <iostream>
#include <string>

using namespace std;

class Request {
public:
    string requestStartLine;
    string method;
    string hostname;
    string port;
Request(){};
    //Request(string line, string m, string h, string p):requestStartLine(line),method(m),hostname(h),port(p){}
    string getMethod();
    string getHostname();
    string getPort();
    ~Request(){};
};
    
