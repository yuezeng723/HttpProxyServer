#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/beast.hpp>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>

#include "client.hpp"
#include "response.hpp"
#include "request.hpp"

using namespace std;
namespace http = boost::beast::http;
using boost::asio::ip::tcp;

class Filelogger {
private:
    string filename;
    ofstream file;
    mutex logLock;

public:
    Filelogger(const string &filename) {
        ofstream temp(filename, ios::app);
        if (!temp.is_open()) {
            throw runtime_error("Cannot open the file");
        }
        file.swap(temp); //copy-and-swap
    }

    ~Filelogger() noexcept {
        try {
            if (file.is_open()) {
                file.close();
            }
        } 
        catch (const exception &e) {
            cerr << "Catch exception in ~Filelogger: " << e.what() << endl;
        }
    }
    
    void logClientConnection(std::shared_ptr<Client> client);
    void logClientRequest(std::shared_ptr<Client> client, http::request<http::string_body> &request);
    void test(std::shared_ptr<Client> client, http::request<http::empty_body> &request);
    void logTunnelClose(std::shared_ptr<Client> client);

    void logProxyRequestToRemote(std::shared_ptr<Client> client,http::request<http::string_body> &request, string &host);
    void logRemoteResponseToProxy(std::shared_ptr<Client> client, http::response<boost::beast::http::dynamic_body> &response, string &host);

    void logProxyResponseToClient(std::shared_ptr<Client> client, http::response<boost::beast::http::dynamic_body> &response);
    void logGETCondition(std::shared_ptr<Client> client, string message);
    void logCacheRequireValidation(std::shared_ptr<Client> client);
    void logCacheExpireAt(std::shared_ptr<Client> client, time_t expireTime);
    void logNotCacheable(std::shared_ptr<Client> client, Response &response); 

    void logNotInCache(std::shared_ptr<Client> client);
    void logInCacheExpire(std::shared_ptr<Client> client, Response &cachedResponse, Request &request, time_t t0);
    void logInCacheValid(std::shared_ptr<Client> client);
    void logInCacheRevalidation(std::shared_ptr<Client> client);
};