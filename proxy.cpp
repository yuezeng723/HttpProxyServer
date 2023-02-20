<<<<<<< proxy.cpp
#include "proxy.hpp"

int Proxy::initializeServerSocket() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Server Initialization Failure: cannot create socket");
        exit(EXIT_FAILURE); 
    }
    if (bind(server_socket,(struct sockaddr *)&server_address,sizeof(server_address)) < 0) { 
        perror("Server Initialization Failure: cannot bind socket"); 
        exit(EXIT_FAILURE); 
    }
    if (listen(server_socket, BACKLOG) < 0) { 
        perror("Server Initialization Failure: cannot listen socket"); 
        exit(EXIT_FAILURE); 
    }
    return server_socket;
}

/*This function referred my last year 650 homework hotpotato*/
string Proxy::parseClientIp(int client_socket) {
    socklen_t len;
    struct sockaddr_storage addr;
    char ipstr[INET_ADDRSTRLEN];
    len = sizeof addr;
    getpeername(client_socket, (struct sockaddr*)&addr, &len);
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    string ip(ipstr);
    return ip;
}

void Proxy::serverListen(int server_socket) {
    struct sockaddr_in client_address;
    unsigned int client_address_len = sizeof(client_address);
    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket < 0) {
            perror("Server Initialization Failure: cannot accept socket"); 
            exit(EXIT_FAILURE); 
        }
        string clientIp = parseClientIp(client_socket);
        Client * client = new Client(clientIp, clientId);
        clientId++;
        client->connectLog();
    }
}

void Proxy::start() {
    int server_socket = initializeServerSocket();
    serverListen(server_socket);
}

