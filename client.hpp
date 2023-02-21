#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <netinet/in.h>

#include "constant.hpp"
using namespace std;
class Client {
private:
    string ip;
    int clientId;
    int client_socket;
    mutex logMutexLock;
public:
    Client(string ip, int clientId, int client_socket): ip(ip), clientId(clientId), client_socket(client_socket){}
    void logConnectMessage();
    int getClientSocket();
};