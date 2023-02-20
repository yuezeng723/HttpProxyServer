#include "buildFunction.h"

int build_serverSocket(const char * portName){
    struct addrinfo hints;
    struct addrinfo *serverInfo;//pointing to filled info
    struct addrinfo *p;//temp pointer used to traverse linked list
    int yes = 1;
    int listener; //listener file-descriptor for any connection
    //fill in address-info
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;//IPv4
    hints.ai_socktype = SOCK_STREAM;//TCP stream
    hints.ai_flags = AI_PASSIVE;//let system fill in my IP automatically
    if (getaddrinfo(NULL, portName, &hints, &serverInfo) != 0) {
        perror("getaddrinfo error: can't get address information!");
        exit(EXIT_FAILURE);
    }
    //create and bind socket
    for(p = serverInfo; p != NULL; p = p->ai_next) {
        //create socket
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket error: can't create socket in server!");
            continue;
        }
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt error: cant set socket!");
            exit(EXIT_FAILURE);
        }
        //bind socket
        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1){
            close(listener);
            perror("bind error: can't bind socket to port!");
            continue;
        }
        break;
    }
    freeaddrinfo(serverInfo);
    if (p == NULL){
        perror("bind error: can't bind socket to port!");
        exit(EXIT_FAILURE);
    }
    //listen socket
    if(listen(listener, BACKLOG) == -1){
        perror("lister error: can't listen socket to client!");
        exit(EXIT_FAILURE);
    }
    return listener;
}