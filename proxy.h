#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <stdio.h>

class proxy{
    private:
        const char * portNum;
    public:
        proxy(const char * port): portNum(port){}
        void run();
};