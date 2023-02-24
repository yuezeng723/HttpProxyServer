#include "proxy.hpp"
#include <vector>

using namespace std;
void Proxy::initializeServerSocket() {
    server_socket = socket(host_info_list->ai_family,
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

void Proxy::serverListen() {
    struct sockaddr_in client_address;
    unsigned int client_address_len = sizeof(client_address);
    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket < 0) {
            perror("Server Initialization Failure: cannot accept socket"); 
            exit(EXIT_FAILURE); 
        }
        string clientIp = parseClientIp(client_socket);
        int newClientId = clientId;
        if (ipToIdMap.find(clientIp) != ipToIdMap.end()) {
            newClientId = ipToIdMap[clientIp];
        } else {
            ipToIdMap[clientIp] = newClientId;
            clientId++;
        }
        Client * client = new Client(clientIp, newClientId, client_socket);
        thread clientHandleThread([this, client](){
            Proxy::handler(client);
        });
        clientHandleThread.join();//TODO: search what the join used for
    }
}

void Proxy::start() {
    serverListen();
}


void Proxy::readRequest(boost::asio::ip::tcp::socket clientSocket) {

}

void handlerRequestHeader(const boost::system::error_code& error){

}

void handleRequestBody(const boost::system::error_code& error){


}

void Proxy::handler(Client* client) {
    client->logConnectMessage();

    //pesudo code
    int server_socket = initializeClientSocket();
    if (requestMethod == 'CONNECT'){
        handleCONNECT(client,server_socket);
    }
    else if(requestMethod == 'GET'){

    }
    else if(requestMethod == 'POST'){

    }
    //vector<char> buffer()
    //recv()
        //from chatgpt

    // vector<char> buffer(1024);
    // string request;
    // while (true) {
    //         int bytesReceived = recv(client->getClientSocket(), buffer.data(), buffer.size(), 0);
    //         if (bytesReceived == 0) {
    //             break;
    //         } else if (bytesReceived <= 0) {
    //             perror("Wong! recv");
    //             exit(EXIT_FAILURE);
    //         } else {
    //             request.append(buffer.data(), bytesReceived);
    //             if (request.find("\r\n\r\n") != std::string::npos) {
    //                 break;
    //             }
    //         }
    //     }
    // cout << request;
    close(client->getClientSocket());
    close(server_socket);
}

void handleCONNECT(Client * client, int server_socket){
    send(client->getClientSocket(), "HTTP/1.1 200 OK\r\n\r\n", 19, 0);
    //select
}

void handlePOST(Client * client, int server_socket){
    //send request
    //send response
}

void handle

git 

