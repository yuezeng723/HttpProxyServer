#include "proxy.hpp"
#include <vector>
using namespace std;

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
Request Proxy::readRequest(Client * client) {
    Request request;
    boost::asio::streambuf requestBuffer;
    //the method below read the **request header** into the requestBuffer
    boost::asio::read_until(client->getClientSocket(), requestBuffer, "\r\n\r\n");
    istream requestSteam(&requestBuffer);
    string requestStartLine;
    getline(requestSteam, requestStartLine);
    logRequestStartline(requestStartLine);
    request.requestStartLine = requestStartLine;
    //NOTICE: For the POST request, I haven't read the request body yet.
    //please implement it in whatever way you want :)

    //parse method from requestStartLine
    int pos_space1 = requestStartLine.find(" ");
    string method = requestStartLine.substr(0,pos_space1);
    //cout << "method: " << method;//???
    request.method = method;
    logRequestStartline(method);
    

    //parse hostname
    if(method == "CONNECT"){
    // int pos_colon1 = requestStartLine.find(":");
    // string method = requestStartLine.substr(pos_space1+1,);
    }
    //parse
    

    return request;
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

    Request request = readRequest(client);
    //cout <<"Get Method" << request.method;//???
    if(request.method == "CONNECT"){

    }
    else if(request.method == "GET"){

    }
    else if(request.method == "POST"){

    }
    delete client;
}
/*
    //pesudo code

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
*/
void handleCONNECT(Client * client, int server_socket){
    //send response
    boost::asio::write(client->getClientSocket(), "HTTP/1.1 200 OK\r\n\r\n");
    send(client->getClientSocket(), "HTTP/1.1 200 OK\r\n\r\n", 19, 0);//???
    //select
}

void handlePOST(Client * client, int server_socket){
    //send request
    //send response
}

void handleGET(Client * client, int server_socket){

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

