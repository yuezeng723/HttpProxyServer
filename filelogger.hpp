#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>

#include "client.hpp"
using namespace std;

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
    
    void logClientConnection(Client * client);

};