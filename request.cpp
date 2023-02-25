#include "request.hpp"

using namespace std;

void Request::sepHostPort(){
    int posColon = url.find(":");

    if(posColon != string::npos){
    hostname = url.substr(0,posColon-1);
    port = url.substr(posColon+1,url.length());
    }
}