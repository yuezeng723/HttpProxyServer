#include <arpa/inet.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/beast.hpp>
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
// #include "client.hpp"
#include "filelogger.hpp"
#include "request.hpp"
#include "cache.hpp"

using namespace std;
namespace http = boost::beast::http;
using boost::asio::ip::tcp;

class Proxy {
private:
   struct addrinfo host_info;
   struct addrinfo *host_info_list;
   const char * hostname = NULL;
   const char * portnumber;
   int server_socket;//the file descriptor of server listen socket
   int clientId;
   unordered_map<string, int> ipToIdMap;//key: client's ip ; value: client's id
   Filelogger logger;
   LRUCache<string, pair<string, http::response<http::dynamic_body>>> cache;

   mutex logMutexLock;//a read-write lock to protect proxy.log file.
public:
    Proxy():logger("./proxy.log"), cache(10){
        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags = AI_PASSIVE;
        portnumber = DEFAULTPORT;
        if (getaddrinfo(hostname, portnumber, &host_info, &host_info_list) != 0) {
            cerr << "Error: cannot get address info for the host" << endl;
            cerr << " (" << hostname << "," << portnumber << ")" << endl;
        }
        clientId = 0;
        initializeServerSocket();
    }
    Proxy(string port, size_t cacheSize): portnumber(port.c_str()), logger("./proxy.log"), cache(cacheSize){
        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags = AI_PASSIVE;
        portnumber = DEFAULTPORT;
        if (getaddrinfo(hostname, portnumber, &host_info, &host_info_list) != 0) {
            cerr << "Error: cannot get address info for the host" << endl;
            cerr << " (" << hostname << "," << portnumber << ")" << endl;
        }
        clientId = 0;
        initializeServerSocket();
    }
    ~Proxy() {
        close(server_socket);
    }

private:
    void initializeServerSocket();
    void serverListen();
    string parseClientIp(int client_socket);
    void handler(Client* client);
    void handleRequest(Client * client);
    void handleConnect(Client * client, boost::beast::flat_buffer& clientBuffer, string requestTarget);
    Request parseRequestHeader(string requestStartLine);
    void logRequestStartline(string startline);
    void parseHostnameAndPort(const std::string& requestTarget, string &hostname, string &port);

public:
    void start();
};