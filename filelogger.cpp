#include "filelogger.hpp"

void Filelogger::logClientConnection(Client * client) {
    lock_guard<mutex> lock(logLock);
    file << "Client connect to the server. Client ID: " << client->getId() << " Client IP: " << client->getIp()  << endl;
}