#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
using namespace std;
class Client {
private:
    string ip;
    int clientId;
public:
    Client(string ip, int clientId): ip(ip), clientId(clientId) {}
    void connectLog();
};