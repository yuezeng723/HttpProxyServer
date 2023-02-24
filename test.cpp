#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

void handle_proxy(tcp::socket& client_socket, const std::string& remote_host, const std::string& remote_port)
{
    try {
        boost::asio::io_context io_context;

        // Resolve the remote host and port number
        tcp::resolver resolver(io_context);
        tcp::resolver::query query(remote_host, remote_port);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        // Establish a TCP/IP connection to the remote host
        tcp::socket remote_socket(io_context);
        boost::asio::connect(remote_socket, endpoint_iterator);

        // Respond to the client with a 200 Connection Established response
        boost::asio::write(client_socket, boost::asio::buffer("HTTP/1.1 200 Connection Established\r\n\r\n"));

        // Relay traffic between the client and the remote host
        boost::asio::streambuf client_buffer, remote_buffer;
        while (true) {
            boost::asio::async_read_until(client_socket, client_buffer, "\r\n\r\n",
                [&client_socket, &remote_socket, &remote_buffer](const boost::system::error_code& error, std::size_t bytes_transferred) {
                    if (!error) {
                        boost::asio::async_write(remote_socket, client_buffer,
                            [&client_socket, &remote_socket, &client_buffer](const boost::system::error_code& error, std::size_t bytes_transferred) {
                                if (!error) {
                                    boost::asio::async_read(remote_socket, remote_buffer,
                                        [&client_socket, &remote_socket, &client_buffer, &remote_buffer](const boost::system::error_code& error, std::size_t bytes_transferred) {
                                            if (!error) {
                                                boost::asio::async_write(client_socket, remote_buffer,
                                                    [&client_socket, &remote_socket, &client_buffer, &remote_buffer](const boost::system::error_code& error, std::size_t bytes_transferred) {
                                                        if (!error) {
                                                            handle_proxy(client_socket, remote_host, remote_port);
                                                        }
                                                    });
                                            }
                                        });
                                }
                            });
                    }
                });
            return;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;
        return 1;
    }

    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), std::stoi(argv[1])));

        while (true) {
            tcp::socket client_socket(io_context);
            acceptor.accept(client_socket);

            // Read the CONNECT request from the client
            boost::asio::streambuf request_buffer;
            boost::asio::read_until(client_socket, request_buffer, "\r\n\r\n");
            std::string request_string = boost::asio::buffer_cast<const char*>(request_buffer.data());
            std::size_t host_start = request_string.find("CONNECT ") + 8;
            std::size_t host_end = request_string.find(':', host_start);
            std::string remote_host = request_string.substr(host_start, host_end - host_start);
            std::size_t port_end = request
