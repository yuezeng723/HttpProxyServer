#include <arpa/inet.h>
#include <cstring>
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
#include <time.h>
#include <unistd.h>

#include "constant.hpp"
#include "client.hpp"
using namespace std;

class Proxy {
private:
   int portnumber;
   struct sockaddr_in server_address;
   int clientId;
public:
    Proxy(){
        memset(server_address.sin_zero, '\0', sizeof server_address.sin_zero); 
        server_address.sin_family = AF_INET; 
        server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
        server_address.sin_port = htons(DEFAULTPORT);
        clientId = 1;
    }
    Proxy(int port): portnumber(port) {
        memset(server_address.sin_zero, '\0', sizeof server_address.sin_zero); 
        server_address.sin_family = AF_INET; 
        server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
        server_address.sin_port = htons(port);
        clientId = 1;
    }
private:
    int initializeServerSocket();
    void serverListen(int server_socket);
    string parseClientIp(int server_socket);
    void handler()
public:
    void start();
};