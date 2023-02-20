int createSocket();
void initializeListenSocket(int server_fd, struct sockaddr_in address, int addrlen);
int createCommunicationSocket(int server_fd,  struct sockaddr_in address, int addrlen);
void greetToClient(int communicate_fd);