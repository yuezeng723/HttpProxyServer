#include "request.hpp"

string Request::getMethod(){
    return method;
}

string Request::getHostname(){
    return hostname;
}
    
string Request::getPort(){
    return port;
}