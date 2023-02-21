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
    //parse http
    //1. 收到browser request： connect
        // 1.1 收到browser request： get
            // 1.2 我们在cache里找有没有对应的资源，如果有
                //1.2.1 查看cache里面的资源有没有过期
                    //1.2.1.1 如果过期了，我们和目标server 建立socket，转发request索要资源，
                    //然后再把拿到的资源存进cache里面，然后再发给browser
                    //1.2.1.2 如果没有过期， 我们就把cache里的资源用response的形势发给browser
            // 1.3 我们在cache里没有找到对应的资源
                // 1.3.1 proxy和目标server 建立socket，转发request索要资源，然后再把拿到的资源存进cache里面，然后再发给browser

        // 收到browser request： post
            // 直接转发给目标server
            // 然后我们收到目标server发来的response，直接转发response给browser

        //  /var/log/erss/proxy.log
}
