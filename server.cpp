#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/fcntl.h>
#include <iostream>
#include "constant.hpp"
using namespace std;
/*Create socket*/
int createSocket() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Server Initialization Failure: cannot create socket");
        exit(EXIT_FAILURE); 
    }
    return server_fd;
}
/*Bind socket*/
void initializeListenSocket(int server_fd, struct sockaddr_in address, int addrlen) { 
    if (bind(server_fd,(struct sockaddr *)&address,sizeof(address)) < 0) { 
        perror("Server Initialization Failure: cannot bind socket"); 
        exit(EXIT_FAILURE); 
    }
    if (listen(server_fd, BACKLOG) < 0) { 
        perror("Server Initialization Failure: cannot listen socket"); 
        exit(EXIT_FAILURE); 
    }
}

/*Accept new connection and create communication socket*/
void createCommunicationSocket(int server_fd,  struct sockaddr_in address, int addrlen) {
    while (1) {
        int communicate_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (communicate_fd < 0) {
            perror("Server Initialization Failure: cannot accept socket"); 
            exit(EXIT_FAILURE); 
        }
        char buffer[20000] = {0};
        const char* hello = "Hello from the server!";
        long valread = read(communicate_fd, buffer, 20000);
        printf("%s\n", buffer);
        write(communicate_fd, hello, strlen(hello));
        close(communicate_fd);
    }
}
