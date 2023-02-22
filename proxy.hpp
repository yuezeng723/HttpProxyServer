#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <mutex>
#include <time.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include "constant.hpp"
#include "client.hpp"
using namespace std;

class Proxy {
private:
   struct addrinfo host_info;
   struct addrinfo *host_info_list;
   const char * hostname = NULL;
   const char * portnumber;
   int clientId;
   mutex logMutexLock;
public:
    Proxy(){
        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags = AI_PASSIVE;
        portnumber = DEFAULTPORT;
        if (getaddrinfo(hostname, portnumber, &host_info, &host_info_list) != 0) {
            cerr << "Error: cannot get address info for the host" << endl;
            cerr << " (" << hostname << "," << portnumber << ")" << endl;
        }
        clientId = 1;
    }
    Proxy(string port): portnumber(port.c_str()) {
        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags = AI_PASSIVE;
        portnumber = DEFAULTPORT;
        if (getaddrinfo(hostname, portnumber, &host_info, &host_info_list) != 0) {
            cerr << "Error: cannot get address info for the host" << endl;
            cerr << " (" << hostname << "," << portnumber << ")" << endl;
        }
        clientId = 1;
    }
private:
    int initializeServerSocket();
    void serverListen(int server_socket);
    string parseClientIp(int server_socket);
    static void handler(void * argument);

public:
    void start();
};