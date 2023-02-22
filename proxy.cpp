#include "proxy.hpp"

int Proxy::initializeServerSocket() {
    int server_socket = socket(host_info_list->ai_family,
                            host_info_list->ai_socktype,
                            host_info_list->ai_protocol);
    if (server_socket < 0) {
        cerr << "Server Initialization Failure: cannot create socket" << endl;
        exit(EXIT_FAILURE); 
    }

    int yes = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        cerr << "Server Initialization Failure: cannot set socket option" << endl;
        exit(EXIT_FAILURE); 
    }
    if (bind(server_socket, host_info_list->ai_addr, host_info_list->ai_addrlen) == -1) { 
        cerr << "Server Initialization Failure: cannot bind socket" << endl; 
        exit(EXIT_FAILURE); 
    }   
    if (listen(server_socket, BACKLOG) == -1) { 
        cerr << "Server Initialization Failure: cannot listen socket" << endl; 
        exit(EXIT_FAILURE); 
    }
    freeaddrinfo(host_info_list);
    return server_socket;
}

/**
 * Parse client's ip address
 * @return client's ip address stored in string format
*/
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
        Client * client = new Client(clientIp, clientId, client_socket);
        clientId++;
        thread clientHandleThread(&handler, client);
        clientHandleThread.join();//TODO: search 
    }
}

void Proxy::start() {
    int server_socket = initializeServerSocket();
    serverListen(server_socket);
}

void Proxy::handler(void* argument) {
    Client * client = (Client *) argument;
    client->logConnectMessage();
    //vector<char> buffer()
    //recv()
    
}
