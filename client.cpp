#include "client.hpp"

/**
 * Log client id and ip address into target log file.
*/
void Client::logConnectMessage() {
    lock_guard<mutex> lock(logMutexLock);
    ofstream logfile(LOG_FILE, ios::app); // LOG_FILE is defined in constant.hpp
    if (logfile.is_open()) {
        logfile << "Client connect to the server. Client ID: " << clientId << " Client IP: " << ip  << endl;
        logfile.close();
    } else {
        cerr << "Error: Could not open log file for writing." << endl;
        exit(EXIT_FAILURE);
    }
}

/**
 * Getter for client_socket
 * @return client_socket is the file descriptor that you can read / write messages in it.
*/
int Client::getClientSocket() {
    return client_socket;
}