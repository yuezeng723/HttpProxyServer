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
    getpeername(client_socket, (struct sockaddr *)&addr, &len);
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
            cerr << "Server Initialization Failure: cannot accept socket" << endl;
            exit(EXIT_FAILURE);
        }
        string clientIp = parseClientIp(client_socket);
        int newClientId = clientId;
        clientId++;
        std::shared_ptr<Client> client( new Client(clientIp, newClientId, client_socket));
        thread clientHandleThread([this, client]() {
            Proxy::handler(client);
        });
        clientHandleThread.detach();
    }
}

/**
 * only public method for starting proxy service
 */
void Proxy::start() {
    serverListen();
}

// feel free to modify this function
void Proxy::handleRequest(std::shared_ptr<Client>client) {
    try {
        // Read the request line from the client
        boost::beast::flat_buffer clientBuffer;
        http::request<http::string_body> request;
        http::read(client->getClientSocket(), clientBuffer, request);
        // Parse the request method
        string method = request.method_string().to_string();
        string requestTarget = string(request.target().data(), request.target().length());
        if (method == "CONNECT") {
            logger.logClientRequest(client, request);
            handleConnect(client, clientBuffer, requestTarget);
        } else if (method == "GET") {
            logger.logClientRequest(client, request);
            handleGet(client, request);
        } else if (method == "POST") {
            logger.logClientRequest(client, request);
            handlePost(client, clientBuffer, request);
        } else {
            http::response<http::string_body> response{
                http::status::bad_request, request.version()};
            response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            response.keep_alive(false);
            response.body() = "Unsupported request method";
            response.prepare_payload();
            http::write(client->getClientSocket(), response);
        }
    } catch (exception &e) {
        cerr << "Error in handleRequest: " << e.what() << endl;
    }
}

/**
 * A handler for multi-threading. New thread does the thing in handler
 */
void Proxy::handler(std::shared_ptr<Client>client) {
    handleRequest(client);
}

void Proxy::parsePort(http::request<http::string_body> &request, string &port){
    boost::beast::string_view target = request.target();
    std::size_t colon_pos = target.find_last_of(':');
    std::size_t slash_pos = target.find_last_of('/');
    if (colon_pos != boost::beast::string_view::npos &&
        slash_pos != boost::beast::string_view::npos &&
        colon_pos > slash_pos) {
    std::string port_str = target.substr(colon_pos + 1, slash_pos - colon_pos - 1).to_string();
    int port = std::stoi(port_str);
    }
}

void Proxy::handlePost(std::shared_ptr<Client>client, boost::beast::flat_buffer &clientBuffer, http::request<http::string_body> &request) {
    
        boost::system::error_code error;
        string hostname;
        string port = "80";
        auto host = request[http::field::host];
        hostname = host.to_string();
        parsePort(request, port);
        tcp::resolver resolver(client->getClientSocket().get_executor());
        tcp::resolver::query query(hostname, port);
        tcp::resolver::results_type endpoints = resolver.resolve(query);
        // Connect to the destination server
        tcp::socket remoteSocket(client->getClientSocket().get_executor());
        boost::asio::connect(remoteSocket, endpoints);
    try {
        // Send the new request to the destination server
        http::write(remoteSocket, request, error);
        logger.logProxyRequestToRemote(request, hostname);

        // Read the response from the destination server
        boost::beast::flat_buffer remoteBuffer;
        http::response<boost::beast::http::dynamic_body> response;
        http::read(remoteSocket, remoteBuffer, response);
        logger.logRemoteResponseToProxy(response, hostname);

        // Send the response back to the client
        http::write(client->getClientSocket(), response, error);
        logger.logProxyResponseToClient(response);

        // close both sockets
        remoteSocket.shutdown(tcp::socket::shutdown_both, error);
        remoteSocket.close();
        logger.logTunnelClose(client);
    } catch (std::exception const &e) {
        std::cerr << "Error in handlePost: " << e.what() << std::endl;
    }
}

void Proxy::handleConnect(std::shared_ptr<Client>client, boost::beast::flat_buffer &clientBuffer, string requestTarget) {
    
    boost::system::error_code error;
    // Parse the hostname and port from the CONNECT request target
    string hostname;
    string port;
    parseHostnameAndPort(requestTarget, hostname, port, "Connect");
    // Resolve the hostname to an endpoint
    tcp::resolver resolver(client->getClientSocket().get_executor());
    tcp::resolver::query query(hostname, port);
    tcp::resolver::results_type endpoints = resolver.resolve(query);
    // Build a socket to the target server and connect to the target server
    tcp::socket remoteSocket(client->getClientSocket().get_executor());
    boost::asio::connect(remoteSocket, endpoints);
    try{
        // Send 200 OK response to the client
        http::response<http::dynamic_body> response{boost::beast::http::status::ok, 11};
        http::write(client->getClientSocket(), response, error);
        logger.logProxyResponseToClient(response);
        // Forward data between the client and the remote server
    
        boost::beast::flat_buffer remoteBuffer;
        while (1) {
            // Read data from the client
            size_t clientLength = boost::asio::read(client->getClientSocket(), clientBuffer, boost::asio::transfer_at_least(1), error);
            if (error == boost::asio::error::eof) {
                // The client has closed the connection
                break;
            } else if (error) {
                // An error occurred
                throw boost::system::system_error(error);
            }
            // Forward the client data to the remote server
            boost::asio::write(remoteSocket, clientBuffer.data(), error);
            // Read data from the remote server
            size_t remoteLength = boost::asio::read(remoteSocket, remoteBuffer, boost::asio::transfer_at_least(1), error);
            if (error == boost::asio::error::eof) {
                // The remote server has closed the connection
                break;
            } else if (error) {
                // An error occurred
                throw boost::system::system_error(error);
            }
            // Forward the remote server data to the client
            http::response<http::vector_body<char>> remoteResponse;
            remoteResponse.body().resize(remoteLength);
            std::memcpy(remoteResponse.body().data(), remoteBuffer.data().data(), remoteLength);
            remoteResponse.set(http::field::content_length, to_string(remoteLength));
            // remoteResponse.keep_alive(false);
            http::write(client->getClientSocket(), remoteResponse, error);
        }
        // Close the remote socket
        remoteSocket.shutdown(tcp::socket::shutdown_both, error);
        remoteSocket.close();
        logger.logTunnelClose(client);
    } catch (exception &e) {
        cerr << "CONNECT request error: " << e.what() << endl;
        boost::system::error_code error;
        client->getClientSocket().shutdown(tcp::socket::shutdown_both, error);
        client->getClientSocket().close();
        remoteSocket.shutdown(tcp::socket::shutdown_both, error);
        remoteSocket.close();
        client->getClientSocket().close();
    }
}

void Proxy::parseHostnameAndPort(const std::string &requestTarget, string &hostname, string &port, string method) {
    string::size_type pos = requestTarget.find(':');
    if (pos != string::npos) {
        hostname = requestTarget.substr(0, pos);
        port = requestTarget.substr(pos + 1);
    } else {
        hostname = requestTarget;
        if (method == "connect") {
            port = "443";
        } else if (method == "get") {
            port = "80";
        } else if (method == "get") {
            port = "8080";
        }
    }
}

void Proxy::storeTime(string request) {
    lock_guard<mutex> lock(mutexLock);
    time_t responseTime = time(0);
    validTime.insert({request, responseTime});
}

void Proxy::removeTime(string &key) {
    lock_guard<mutex> lock(mutexLock);
    validTime.erase(key);
}

time_t Proxy::getTime(string &key) {
    lock_guard<mutex> lock(mutexLock);
    time_t time = validTime[key];
    return time;
}
void Proxy::revalidateReq(Response &resInfo, http::request<http::string_body> &request) {
    // Add headers
    lock_guard<mutex> lock(mutexLock);
    if (resInfo.getETAG() != "") {
        request.set(http::field::if_none_match, resInfo.getETAG());
    }
    if (resInfo.getLastModify() != "") {
        request.set(http::field::if_modified_since, resInfo.getLastModify());
    }
}


void Proxy::handleChunked(http::response<http::dynamic_body> &newResponse, boost::beast::flat_buffer &serverBuffer, tcp::socket &remoteSocket) {
    boost::beast::error_code error;
    http::response_parser<http::dynamic_body> parser;
    string chunk;
    size_t size = 0;
    auto body_cb =
[&](std::uint64_t remain,   // The number of bytes left in this chunk
    boost::beast::string_view body,       // A buffer holding chunk body data
    boost::beast::error_code& ec)         // We can set this to indicate an error
    {
        if(remain == body.size()) ec = http::error::end_of_chunk;
        chunk.append(body.data(), body.size());
        size += body.size();
        return body.size();
    };
    parser.on_chunk_body(body_cb);
    while (!parser.is_done()) {
        http::read(remoteSocket, serverBuffer, parser, error);
        if( !error) continue;
        else if ( error!=http::error::end_of_chunk) break;
        else error = {};
    }
}


//当在cache中找到response后，检查这个response是否过期
string Proxy::checkValidation(http::response<http::dynamic_body> &cachedResponse, http::request<http::string_body> &clientRequest, string &key) {
    Response response(cachedResponse);
    Request request(clientRequest);
    time_t currTime = time(0);

    time_t t0 = getTime(key);
    if (response.noCache) return "revalidate";
    if (request.has_min_fresh && response.max_age != 0) {
        if (currTime < t0 + response.max_age - request.min_fresh) return "valid";
        else if (currTime < t0 + response.max_age) return "revalidate";
        else return "expire";
    }
    if (response.mustRevalidate && response.max_age != 0) {
        if (currTime < t0 + response.max_age) return "valid";
        else return "revalidate";
    }
    if (request.has_max_stale && response.max_age != 0) {
        if (currTime < t0 + response.max_age + request.max_stale) return "valid";
        else return "expire";
    }
    return "expire";
}

bool Proxy::checkNeedCache(http::response<http::dynamic_body> &serverResponse) {
    Response response(serverResponse);
    if (response.pri || response.noStore) return false;
    return true;
}

//revalidate 的时候用
void Proxy::handleRemote200Ok(std::shared_ptr<Client> client, http::response<http::dynamic_body> &remoteResponse, http::request<http::string_body>&clientRequest,string &key) {
    try{
        boost::beast::error_code error;
        Response newRes(remoteResponse);
        Request request(clientRequest);
        bool needCache = checkNeedCache(remoteResponse);
        if (needCache == false) {  
            logger.logNotCacheable(client, newRes);
        } 
        else {  
            cacheResponse(client, clientRequest, remoteResponse, key);
        }
        http::write(client->getClientSocket(), remoteResponse, error);
        logger.logProxyResponseToClient(remoteResponse);
    }
    catch (exception &e){
        cerr << "Error in handleRemote200Ok: " << e.what() << endl;
    }
}


void Proxy::handleGet(std::shared_ptr<Client> client, http::request<http::string_body> &clientRequest) {
    
    boost::beast::error_code error;
    string hostname;
    string port = "80";
    string requestTarget = string(clientRequest.target().data(), clientRequest.target().length());
    auto host = clientRequest[http::field::host];
    hostname = host.to_string();
    string key = requestTarget + " " + hostname;
    
    // Resolve the hostname to an endpoint
    tcp::resolver resolver(client->getClientSocket().get_executor());
    tcp::resolver::query query(hostname, port);
    tcp::resolver::results_type endpoints = resolver.resolve(query);

    // Build a socket to the target server and connect to the target server
    tcp::socket remoteSocket(client->getClientSocket().get_executor());
    boost::asio::connect(remoteSocket, endpoints);
    string message = "";
    try{
        boost::beast::flat_buffer remoteBuffer;
        http::response<http::dynamic_body> newResponse;
        bool needCache = false;
        std::shared_ptr<pair<string, http::response<http::dynamic_body>>> target = cache.get(key);
        http::response<http::dynamic_body> &cachedResponse = target->second;
        Request requestObj(clientRequest);
    
        if (target != nullptr) {  // find in cache
            Response cachedRespObj(cachedResponse);
            string validation = checkValidation(cachedResponse,clientRequest, key);
            if (validation == "valid") {
                logger.logInCacheValid(client);
                http::write(client->getClientSocket(), cachedResponse, error);
                logger.logProxyResponseToClient(cachedResponse);
            }
            else if (validation == "revalidate") {
                logger.logInCacheRevalidation(client);
                revalidateReq(cachedRespObj, clientRequest);
                http::write(remoteSocket, clientRequest, error);  // send request to target server
                logger.logProxyRequestToRemote(clientRequest, hostname);
                // receive new response
                http::read(remoteSocket, remoteBuffer, newResponse);
                //if(newResponse.chunked())handleChunked(newResponse, remoteBuffer, remoteSocket);/****************************/
                logger.logRemoteResponseToProxy(newResponse, hostname);
                //check newResponse status code
                Response newRespObj(newResponse);
                needCache = checkNeedCache(newResponse);
                if (newRespObj.status_code == 304) {
                    http::write(client->getClientSocket(), cachedResponse, error);
                    logger.logProxyResponseToClient(cachedResponse);
                }
                else if (newRespObj.status_code == 200) {
                    handleRemote200Ok(client, newResponse, clientRequest, key);
                }
                else {//得处理remote server返回既不是304 也不是200的情况
                    http::write(client->getClientSocket(), newResponse, error);
                    logger.logProxyResponseToClient(newResponse);
                    if (needCache) {
                        cache.remove(key);
                        removeTime(key);
                        cacheResponse(client, clientRequest, newResponse, key);
                    } else {
                        Response newRespObj(newResponse);
                        logger.logNotCacheable(client, newRespObj);
                    }
                } 
            }
            else {//"expire"
                time_t t0 = getTime(key);
                logger.logInCacheExpire(client, cachedRespObj, requestObj, t0);
                http::write(remoteSocket, clientRequest, error);
                logger.logProxyRequestToRemote(clientRequest, hostname);
                http::read(remoteSocket, remoteBuffer, newResponse);
                //if(newResponse.chunked())handleChunked(newResponse, remoteBuffer, remoteSocket);/**********************/
                logger.logRemoteResponseToProxy(newResponse, hostname);
                needCache = checkNeedCache(newResponse);
                Response newRes(newResponse);
                if (needCache) {//cache it
                    cache.remove(key);
                    removeTime(key);
                    cacheResponse(client, clientRequest, newResponse, key);
                } else {
                    Response newRespObj(newResponse);
                    logger.logNotCacheable(client, newRespObj);
                }
                http::write(client->getClientSocket(), newResponse, error);
                logger.logProxyResponseToClient(newResponse);
            }
        }
        else {//cache没存过此request对应的response
            //redirect the request
            http::write(remoteSocket, clientRequest, error);
            logger.logProxyRequestToRemote(clientRequest, hostname);
            http::read(remoteSocket, remoteBuffer, newResponse);
            //if(newResponse.chunked())handleChunked(newResponse, remoteBuffer, remoteSocket);/**************/
            logger.logRemoteResponseToProxy(newResponse, hostname);
            needCache = checkNeedCache(newResponse);
            if (needCache) {
                cacheResponse(client, clientRequest, newResponse, key);
            } else {
                Response newRespObj(newResponse);
                logger.logNotCacheable(client, newRespObj);
            }
            http::write(client->getClientSocket(), newResponse, error);
            logger.logProxyResponseToClient(newResponse);
        }
        remoteSocket.shutdown(tcp::socket::shutdown_both, error);
        remoteSocket.close();
        client->getClientSocket().shutdown(tcp::socket::shutdown_both, error);
        client->getClientSocket().close();
    }
    catch (exception &e) {
        cerr << "GET request error: " << e.what() << endl;
        boost::system::error_code error;
        client->getClientSocket().shutdown(tcp::socket::shutdown_both, error);
        client->getClientSocket().close();
        remoteSocket.shutdown(tcp::socket::shutdown_both, error);
        remoteSocket.close();
    }
}

void Proxy::cacheResponse(std::shared_ptr<Client> client, http::request<http::string_body> &clientRequest, http::response<http::dynamic_body> &remoteResponse, string &key) {
    time_t currTime = time(0);
    pair<string, http::response<http::dynamic_body>> newRsc = make_pair(key, remoteResponse);
    cache.put(key, newRsc);
    storeTime(key);
    logger.logProxyResponseToClient(remoteResponse);
    Response newRes(remoteResponse);
    Request request(clientRequest);
    if (newRes.noCache || (newRes.max_age == 0)) {
        logger.logCacheRequireValidation(client);
    } 
    else if (request.has_min_fresh && newRes.max_age!=0){
        logger.logCacheExpireAt(client, currTime + newRes.max_age);
    }
    else if (request.has_max_stale && newRes.max_age != 0) {
        logger.logCacheExpireAt(client, currTime + newRes.max_age + request.max_stale);
    }
}

