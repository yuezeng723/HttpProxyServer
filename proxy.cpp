#include "proxy.hpp"

/**
 * create and setup a server listen socket
*/
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

/**
 * listen to incomming connect client
 * update the ipToId map and start multi-threading
*/
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
            Proxy::handler(client);// multi-threading starts
        });
        clientHandleThread.join();//TODO: replace by thread(whaterver).detach();
    }
}

/**
 * only public method for starting proxy service
*/
void Proxy::start() {
    serverListen();
}

//feel free to modify this function
void Proxy::readRequest(Client * client) {
    boost::asio::streambuf requestBuffer;
    //the method below read the **request header** into the requestBuffer
    boost::asio::read_until(client->getClientSocket(), requestBuffer, "\r\n\r\n");
    istream requestSteam(&requestBuffer);
    string requestStartLine;
    getline(requestSteam, requestStartLine);
    logRequestStartline(requestStartLine);
    //NOTICE: For the POST request, I haven't read the request body yet.
    //please implement it in whatever way you want :)

}

//feel free to modify this funtion or delete it ;)
void Proxy::handlerRequestHeader(const boost::system::error_code& error){

}

//feel free to modify this funtion or delete it ;)
void Proxy::handleRequestBody(const boost::system::error_code& error){

}

/**
 * A handler for multi-threading. New thread does the thing in handler
*/
void Proxy::handler(Client* client) {
    client->logConnectMessage();
    readRequest(client);
    delete client;
}

//print the first line of a request. Feel free to modify this function
void Proxy::logRequestStartline(string startline) {
    lock_guard<mutex> lock(logMutexLock);
    ofstream logfile(LOG_FILE, ios::app); // LOG_FILE is defined in constant.hpp
    if (logfile.is_open()) {
        logfile << startline << endl;
        logfile.close();
    } else {
        cerr << "Error: Could not open log file for writing." << endl;
        exit(EXIT_FAILURE);
    }
}

