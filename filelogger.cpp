#include "filelogger.hpp"

void Filelogger::logClientConnection(Client * client) {
    lock_guard<mutex> lock(logLock);
    file << "Client connect to the server. Client ID: " << client->getId() << " Client IP: " << client->getIp()  << endl;
}

void Filelogger::logClientRequest(Client * client, http::request<http::string_body> &request) {
    lock_guard<mutex> lock(logLock);
    boost::beast::string_view firstLine = request.target();

    //get current time
    time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    file << client->getId() << ": " << "\"" << request.method() << " "<< firstLine <<  "\"" << " from " << client->getIp() << " @ " << ctime(&currentTime);
}

void Filelogger::test(Client * client, http::request<http::empty_body> &request) {
    lock_guard<mutex> lock(logLock);
    file << "Test: Client Id is: " << client->getId() << " request is " << request.target() << endl;
}

void Filelogger::logTunnelClose(Client * client) {
    lock_guard<mutex> lock(logLock);
    file << client->getId() << ": " << "Tunnel closed" << endl;
}