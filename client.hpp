#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <netinet/in.h>

#include "constant.hpp"

using namespace std;
using boost::asio::ip::tcp;

class Client {
private:
    string ip;
    int clientId;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket clientSocket;
    mutex logMutexLock;
public:
    Client(string ip, int clientId, int client_socket): ip(ip), clientId(clientId), clientSocket(io_context) {
        clientSocket.assign(boost::asio::ip::tcp::v4(), client_socket);
    }
    void logConnectMessage();
    boost::asio::ip::tcp::socket& getClientSocket();

};