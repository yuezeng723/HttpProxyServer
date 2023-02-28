#include "filelogger.hpp"

void Filelogger::logClientConnection(std::shared_ptr<Client> client) {
    lock_guard<mutex> lock(logLock);
    file << "Client connect to the server. Client ID: " << client->getId() << " Client IP: " << client->getIp()  << endl;
}

void Filelogger::logClientRequest(std::shared_ptr<Client> client, http::request<http::string_body> &request) {
    lock_guard<mutex> lock(logLock);
    boost::beast::string_view firstLine = request.target();
    //get current time
    time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    file << client->getId() << ": " << "\"" << request.method() << " "<< firstLine <<  "\"" << " from " << client->getIp() << " @ " << ctime(&currentTime);
}

void Filelogger::test(std::shared_ptr<Client> client, http::request<http::empty_body> &request) {
    lock_guard<mutex> lock(logLock);
    file << "Test: Client Id is: " << client->getId() << " request is " << request.target() << endl;
}

void Filelogger::logTunnelClose(std::shared_ptr<Client> client) {
    lock_guard<mutex> lock(logLock);
    file << client->getId() << ": " << "Tunnel closed" << endl;
}

void Filelogger::logProxyRequestToRemote( http::request<http::string_body> &request, string &host) {
    lock_guard<mutex> lock(logLock);
    boost::beast::string_view firstLine = request.target();
    file << "Requesting " << "\"" << request.method() << " "<< firstLine <<  "\"" << " from " << host << endl;
}

void Filelogger::logRemoteResponseToProxy(http::response<boost::beast::http::dynamic_body> &response, string &host) {
    lock_guard<mutex> lock(logLock);
    std::stringstream ss;
    int major = response.version() / 10;
    int minor = response.version() % 10;
    ss << major << "." << minor;
    boost::beast::string_view firstLine = ss.str() + " " + std::to_string(response.result_int()) + " " + response.reason().to_string();
    file << "Received " << "\"" << "HTTP/"  << firstLine << "\"" << " from " << host << endl;
}

void Filelogger::logProxyResponseToClient(http::response<boost::beast::http::dynamic_body> &response) {
    lock_guard<mutex> lock(logLock);
    std::stringstream ss;
    int major = response.version() / 10;
    int minor = response.version() % 10;
    ss << major << "." << minor;
    boost::beast::string_view firstLine = ss.str() + " " + std::to_string(response.result_int()) + " " + response.reason().to_string();
    file << "Responding " << "\"" <<  "HTTP/" << firstLine << "\"" << endl;
}

void Filelogger::logGETCondition(std::shared_ptr<Client> client, string message) {
    lock_guard<mutex> lock(logLock);
    file << client->getId() << ": " << message << endl;
}

void Filelogger::logCacheRequireValidation(std::shared_ptr<Client> client){
    lock_guard<mutex> lock(logLock);
    file << client->getId()<< ": " <<" cached, but requires revalidation" << endl;
}

void Filelogger::logCacheExpireAt(std::shared_ptr<Client> client, time_t expireTime){
    lock_guard<mutex> lock(logLock);
    file << client->getId()<< ": " << "cached, expires at " << ctime(&expireTime);

}

void Filelogger::logNotCacheable(std::shared_ptr<Client> client, Response &response){
    lock_guard<mutex> lock(logLock);
    string reason ="";
    if (response.noStore) reason = "no-store";
    if (response.pri) reason = "private";
    file << client->getId()<< ": " << "not cacheable because " << reason << endl;
}

void Filelogger::logNotInCache(std::shared_ptr<Client> client){
    lock_guard<mutex> lock(logLock);
    file << client->getId()<< ": " << "not in cache" << endl;
}
void Filelogger::logInCacheExpire(std::shared_ptr<Client> client, Response &cachedResponse, Request &request, time_t t0) {
    lock_guard<mutex> lock(logLock);
    time_t expire;
    if (cachedResponse.max_age!=0 && request.has_min_fresh) expire = t0+cachedResponse.max_age;
    if (cachedResponse.max_age!=0 && request.has_max_stale) expire = t0+cachedResponse.max_age+request.max_stale;
    expire = t0+cachedResponse.max_age;
    file << client->getId()<< ": " << "in cache, but expired at " << ctime(&expire);
}

void Filelogger::logInCacheValid(std::shared_ptr<Client> client) {
    lock_guard<mutex> lock(logLock);
    file << client->getId()<< ": " << "in cache, valid" << endl;
}

void Filelogger::logInCacheRevalidation(std::shared_ptr<Client> client) {
    lock_guard<mutex> lock(logLock);
    file << client->getId()<< ": " << "in cache, requires validation" << endl;
}