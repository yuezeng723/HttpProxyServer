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
        if (ipToIdMap.find(clientIp) != ipToIdMap.end()) {
            newClientId = ipToIdMap[clientIp];
        } else {
            ipToIdMap[clientIp] = newClientId;
            clientId++;
        }
        Client *client = new Client(clientIp, newClientId, client_socket);
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
void Proxy::handleRequest(Client *client) {
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
            getgetget(client, request);
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
        cout << "Error in handleRequest: " << e.what() << endl;
    }
}

/**
 * A handler for multi-threading. New thread does the thing in handler
 */
void Proxy::handler(Client *client) {
    // logger.logClientConnection(client);
    handleRequest(client);
    delete client;
}

void Proxy::handleGet(Client *client, boost::beast::flat_buffer &clientBuffer, http::request<http::string_body> &request) {  // 存取cache的key需要修改？
    try {
        // Parse the hostname and port from the GET request target
        string hostname;
        string port = "80";
        string requestTarget = string(request.target().data(), request.target().length());
        auto host = request[http::field::host];
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

        // http::request<http::string_body> newReq;  // request with "If-None-Match" and "If-Modified-Since" for revalidation
        //  search in cache and find whether there is matched request
        if (cache.get(key) != nullptr) {  // find in cache
            std::shared_ptr<pair<string, http::response<http::dynamic_body>>> target = cache.get(key);
            Response response(target->second);
            if (response.noCache) {  // old response has no-cache, revalidate anyway
                message = "in cache, requires validation";
                logger.logGETCondition(client, message);

                // ！！！！！revalidation可以封装！！！！！！但remotesocket会报错，如何解决？
                //***********revalidation***********************
                http::request<http::string_body> newReq = revalidateReq(response, request);
                http::write(remoteSocket, newReq);  // send request to target server
                logger.logProxyRequestToRemote(newReq, hostname);

                http::response<http::dynamic_body> newResponse;  // receive new response
                boost::beast::flat_buffer serverBuffer;
                http::read(remoteSocket, serverBuffer, newResponse);
                logger.logRemoteResponseToProxy(newResponse, hostname);
                //************* 拼接chunked response *************
                if (newResponse.chunked()) {
                    handleChunked(newResponse, serverBuffer, remoteSocket);
                }
                
                Response newRes(newResponse);
                if (newRes.getStatusCode() == 304) {                         // not modified
                    http::write(client->getClientSocket(), target->second);  // 直接转发旧response
                    logger.logProxyResponseToClient(target->second);
                } else if (newRes.getStatusCode() == 200) {  // return with new full response

                    if (newRes.noStore || newRes.pri) {  // if response has no-store/private, send to browser directly without storing in cache
                        http::write(client->getClientSocket(), newResponse);
                        logger.logProxyResponseToClient(newResponse);
                        if (newRes.noStore) {
                            message = "not cacheable because no-store";
                        } else if (newRes.pri) {
                            message = "not cacheable because private";
                        }
                        logger.logGETCondition(client, message);
                    } else {  // if no no-store/private, store in cache and send to browser

                        pair<string, http::response<http::dynamic_body>> newRsc = make_pair(requestTarget, newResponse);
                        cache.put(key, newRsc);
                        storeTime(key);
                        http::write(client->getClientSocket(), newResponse);
                        logger.logProxyResponseToClient(newResponse);
                        if (newRes.noCache || (newRes.max_age == 0)) {
                            message = "cached, but requires revalidation";
                        } else {
                            message = "cached, expires at " + validTime.find(key)->second + newRes.max_age;
                        }
                        logger.logGETCondition(client, message);
                    }
                }
                //**********************end of revalidation*****************

            } else {  // 旧response没有no-cache
                time_t t0 = validTime.find(key)->second;
                time_t expireTime = t0 + response.max_age;
                time_t maxstaleTime = expireTime + response.max_stale;
                time_t currTime = time(0);
                if (currTime >= t0 && currTime <= expireTime) {  // resource is fresh, send it back to browswer directly
                    message = "in cache, valid";
                    logger.logGETCondition(client, message);
                    http::write(client->getClientSocket(), target->second);
                    logger.logProxyResponseToClient(target->second);
                } else if (currTime >= expireTime && currTime <= maxstaleTime) {  // 不fresh,看是否有must-revalidate
                    if (response.mustRevalidate == 1) {
                        message = "in cache, requires revalidatoin";
                        logger.logGETCondition(client, message);
                        //***********revalidation***********************
                        http::request<http::string_body> newReq = revalidateReq(response, request);
                        http::write(remoteSocket, newReq);  // send request to target server
                        logger.logProxyRequestToRemote(newReq, hostname);

                        http::response<http::dynamic_body> newResponse;  // receive new response
                        boost::beast::flat_buffer serverBuffer;
                        http::read(remoteSocket, serverBuffer, newResponse);
                        logger.logRemoteResponseToProxy(newResponse, hostname);

                        //************* 拼接chunked response *************
                        if (newResponse.chunked()) {
                            handleChunked(newResponse, serverBuffer, remoteSocket);
                        }

                        Response newRes(newResponse);
                        if (newRes.getStatusCode() == 304) {                         // not modified
                            http::write(client->getClientSocket(), target->second);  // 直接转发旧response
                            logger.logProxyResponseToClient(target->second);
                        } else if (newRes.getStatusCode() == 200) {  // return with new full response
                            if (newRes.noStore || newRes.pri) {      // if response has no-store/private, send to browser directly without storing in cache
                                http::write(client->getClientSocket(), newResponse);
                                logger.logProxyResponseToClient(newResponse);
                                if (newRes.noStore) {
                                    message = "not cacheable because no-store";
                                } else if (newRes.pri) {
                                    message = "not cacheable because private";
                                }
                                logger.logGETCondition(client, message);
                            } else {  // if no-store/private, store in cache and send to browser
                                pair<string, http::response<http::dynamic_body>> newRsc = make_pair(key, newResponse);
                                cache.put(key, newRsc);
                                storeTime(key);
                                http::write(client->getClientSocket(), newResponse);
                                logger.logProxyResponseToClient(newResponse);
                                if (newRes.noCache || (newRes.max_age == 0)) {
                                    message = "cached, but requires revalidation";
                                } else {
                                    message = "cached, expires at " + validTime.find(key)->second + newRes.max_age;
                                }
                                logger.logGETCondition(client, message);
                            }
                        }
                        //**********************end of revalidation*****************
                    } else {
                        message = "in cache, but expired at " + expireTime;
                        logger.logGETCondition(client, message);
                        http::write(client->getClientSocket(), target->second);  // 当成valid的
                        logger.logProxyResponseToClient(target->second);
                    }
                } else if (currTime > maxstaleTime) {
                    message = "in cache, requires revalidation";
                    logger.logGETCondition(client, message);
                    //***********revalidation***********************
                    http::request<http::string_body> newReq = revalidateReq(response, request);
                    http::write(remoteSocket, newReq);  // send request to target server
                    logger.logProxyRequestToRemote(newReq, hostname);

                    http::response<http::dynamic_body> newResponse;  // receive new response
                    boost::beast::flat_buffer serverBuffer;
                    http::read(remoteSocket, serverBuffer, newResponse);
                    logger.logRemoteResponseToProxy(newResponse, hostname);
                    Response newRes(newResponse);
                    if (newRes.getStatusCode() == 304) {                         // not modified
                        http::write(client->getClientSocket(), target->second);  // 直接转发旧response
                        logger.logProxyResponseToClient(target->second);
                    } else if (newRes.getStatusCode() == 200) {  // return with new full response
                        handleRemote200Ok(client, newResponse, request, key);
                    }
                    //**********************end of revalidation*****************
                }
            }
        } else {  // do not find in cache
            message = "not in cache";
            logger.logGETCondition(client, message);
            http::write(remoteSocket, request);  // send request to target server
            logger.logProxyRequestToRemote(request, hostname);

            http::response<http::dynamic_body> response;  // receive response
            boost::beast::flat_buffer serverBuffer;
            http::read(remoteSocket, serverBuffer, response);
            logger.logRemoteResponseToProxy(response, hostname);
            Response res(response);
            if (res.getStatusCode() == 200) {
                if (res.noStore || res.pri) {  // if response has no-store/private, send to browser directly without storing in cache
                    if (res.noStore) {
                        message = "not cacheable because no-store";
                    } else if (res.pri) {
                        message = "not cacheable because private";
                    }
                    logger.logGETCondition(client, message);

                    http::write(client->getClientSocket(), response);
                    logger.logProxyResponseToClient(response);

                } else {  // if no-store/private, store in cache and send to browser
                    pair<string, http::response<http::dynamic_body>> newRsc = make_pair(key, response);
                    cache.put(key, newRsc);
                    storeTime(key);

                    if (res.noCache || (res.max_age == 0)) {
                        message = "cached, but requires revalidation";
                    } else {
                        message = "cached, expires at " + validTime.find(key)->second + res.max_age;
                    }
                    logger.logGETCondition(client, message);

                    http::write(client->getClientSocket(), response);
                    logger.logProxyResponseToClient(response);
                }
            }
            // chunk???
        }
        // close both sockets
        boost::system::error_code error;
        remoteSocket.shutdown(tcp::socket::shutdown_both, error);
        remoteSocket.close();
        logger.logTunnelClose(client);

    } catch (std::exception const &e) {
        std::cerr << "Error in handleGet: " << e.what() << std::endl;
    }
}

void Proxy::handlePost(Client *client, boost::beast::flat_buffer &clientBuffer, http::request<http::string_body> &request) {
    try {
        string hostname;
        string port = "80";
        auto host = request[http::field::host];
        hostname = host.to_string();

        tcp::resolver resolver(client->getClientSocket().get_executor());
        tcp::resolver::query query(hostname, port);
        tcp::resolver::results_type endpoints = resolver.resolve(query);
        // Connect to the destination server
        tcp::socket remoteSocket(client->getClientSocket().get_executor());
        boost::asio::connect(remoteSocket, endpoints);

        // Send the new request to the destination server
        http::write(remoteSocket, request);
        logger.logProxyRequestToRemote(request, hostname);

        // Read the response from the destination server
        boost::beast::flat_buffer remoteBuffer;
        http::response<boost::beast::http::dynamic_body> response;
        http::read(remoteSocket, remoteBuffer, response);
        logger.logRemoteResponseToProxy(response, hostname);

        // Send the response back to the client
        http::write(client->getClientSocket(), response);
        logger.logProxyResponseToClient(response);

        // close both sockets
        boost::system::error_code error;
        remoteSocket.shutdown(tcp::socket::shutdown_both, error);
        remoteSocket.close();
        logger.logTunnelClose(client);
    } catch (std::exception const &e) {
        std::cerr << "Error in handlePost: " << e.what() << std::endl;
    }
}

void Proxy::handleConnect(Client *client, boost::beast::flat_buffer &clientBuffer, string requestTarget) {
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
    // Send 200 OK response to the client
    http::response<http::dynamic_body> response{boost::beast::http::status::ok, 11};
    http::write(client->getClientSocket(), response);
    logger.logProxyResponseToClient(response);
    // Forward data between the client and the remote server
    try {
        boost::beast::flat_buffer remoteBuffer;
        boost::system::error_code error;
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
            memcpy(remoteResponse.body().data(), remoteBuffer.data().data(), remoteLength);
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
    time_t responseTime = time(0);
    validTime.insert({request, responseTime});
}

void Proxy::revalidateReq(Response &resInfo, http::request<http::string_body> &request) {
    // Add headers
    if (resInfo.getETAG() != "") {
        request.set(http::field::if_none_match, resInfo.getETAG());
    }
    if (resInfo.getLastModify() != "") {
        request.set(http::field::if_modified_since, resInfo.getLastModify());
    }
}


void Proxy::handleChunked(http::response<http::dynamic_body> &newResponse, boost::beast::flat_buffer &serverBuffer, tcp::socket &remoteSocket) {
    http::response_parser<boost::beast::http::dynamic_body> parser(newResponse);
    cout << "chunked data" << endl;
    if (newResponse.chunked()) {
        cout << "chunked data" << endl;
        while (true) {
        // Check if we have reached the end of the message
            auto transfer_encoding = newResponse.find(boost::beast::http::field::transfer_encoding);
            auto content_length = newResponse.find(boost::beast::http::field::content_length);
            if (transfer_encoding != newResponse.end() && transfer_encoding->value() == "chunked") {
                if (content_length == newResponse.end() || boost::lexical_cast<std::size_t>(content_length->value()) == serverBuffer.size()) {
                    break;
                }
            }
            http::read(remoteSocket, serverBuffer, parser);
        }
        http::response_parser<http::dynamic_body> parsedParser;
        http::response<http::dynamic_body> parsedResponse;
        parsedParser.skip(true);
        boost::beast::error_code error;
        http::read(remoteSocket, serverBuffer, parsedParser, error);
        if (!error) {
            parsedResponse = std::move(parsedParser.get());
        }
        newResponse = std::move(parsedResponse);
    }
}

//当在cache中找到response后，检查这个response是否过期
string Proxy::checkValidation(http::response<http::dynamic_body> &cachedResponse, http::request<http::string_body> &clientRequest, string &key) {
    Response response(cachedResponse);
    Request request(clientRequest);
    time_t currTime = time(0);
    time_t t0 = validTime[key];
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
void Proxy::handleRemote200Ok(Client * client, http::response<http::dynamic_body> &remoteResponse, http::request<http::string_body>&clientRequest,string &key) {
    Response newRes(remoteResponse);
    Request request(clientRequest);
    bool needCache = checkNeedCache(remoteResponse);
    if (needCache == false) {  // if response has no-store/private, send to browser directly without storing in cache
        if (newRes.noStore) { logger.logNotCacheable(client, "no-store");} 
        else if (newRes.pri) { logger.logNotCacheable(client, "private");}
    } 
    else {  // if no no-store/private, store in cache and send to browser
        cacheResponse(client, clientRequest, remoteResponse, key);
    }
    http::write(client->getClientSocket(), remoteResponse);
    logger.logProxyResponseToClient(remoteResponse);
}


void Proxy::getgetget(Client * client, http::request<http::string_body> &clientRequest) {
    
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

    boost::beast::flat_buffer remoteBuffer;
    http::response<http::dynamic_body> newResponse;
    bool needCache = false;
    try{
        if (cache.get(key) != nullptr) {  // find in cache
            std::shared_ptr<pair<string, http::response<http::dynamic_body>>> target = cache.get(key);
            http::response<http::dynamic_body> &cachedResponse = cache.get(key)->second;
            Response cachedRespObj(cachedResponse);
            string validation = checkValidation(cachedResponse,clientRequest, key);
            if (validation == "valid") {
                http::write(client->getClientSocket(), cachedResponse);
            }
            else if (validation == "revalidate") {
                revalidateReq(cachedRespObj, clientRequest);
                http::write(remoteSocket, clientRequest);  // send request to target server
                logger.logProxyRequestToRemote(clientRequest, hostname);
                // receive new response
                http::read(remoteSocket, remoteBuffer, newResponse);
                logger.logRemoteResponseToProxy(newResponse, hostname);
    
                //check newResponse status code
                Response newRespObj(newResponse);
                needCache = checkNeedCache(newResponse);
                if (newRespObj.status_code == 304) {
                    http::write(client->getClientSocket(), cachedResponse);
                    logger.logProxyResponseToClient(cachedResponse);
                }
                else if (newRespObj.status_code == 200) {
                    handleRemote200Ok(client, newResponse, clientRequest, key);
                }
                else {//得处理remote server返回既不是304 也不是200的情况
                    http::write(client->getClientSocket(), newResponse);
                    logger.logProxyResponseToClient(newResponse);
                    //检查是否需要cache，但不想写了
                } 
            }
            else {//"expire"
                http::write(remoteSocket, clientRequest);
                logger.logProxyRequestToRemote(clientRequest, hostname);
                http::read(remoteSocket, remoteBuffer, newResponse);
                logger.logRemoteResponseToProxy(newResponse, hostname);
                needCache = checkNeedCache(newResponse);
                Response newRes(newResponse);
                if (needCache) {//cache it
                    cache.remove(key);
                    cacheResponse(client, clientRequest, newResponse, key);
                }
                http::write(client->getClientSocket(), newResponse);
                logger.logProxyResponseToClient(newResponse);
            }
        }
        else {//cache没存过此request对应的response
            //redirect the request
            http::write(remoteSocket, clientRequest);
            logger.logProxyRequestToRemote(clientRequest, hostname);
            http::read(remoteSocket, remoteBuffer, newResponse);
            logger.logRemoteResponseToProxy(newResponse, hostname);
            needCache = checkNeedCache(newResponse);
            if (needCache) {
                cacheResponse(client, clientRequest, newResponse, key);
            }
            http::write(client->getClientSocket(), newResponse);
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

void Proxy::cacheResponse(Client * client, http::request<http::string_body> &clientRequest, http::response<http::dynamic_body> &remoteResponse, string &key) {
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