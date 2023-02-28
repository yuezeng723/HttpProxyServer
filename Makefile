CXX = g++
CXXFLAGS = -g -std=c++11

all: main

main: main.o proxy.o client.o filelogger.o cache.o response.o request.o
	$(CXX) $(CXXFLAGS) -o main main.o proxy.o client.o filelogger.o cache.o response.o request.o -lpthread

main.o: main.cpp main.hpp proxy.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

proxy.o: proxy.cpp proxy.hpp constant.hpp client.hpp filelogger.hpp cache.hpp response.hpp request.hpp
	$(CXX) $(CXXFLAGS) -c proxy.cpp -lpthread

client.o: client.cpp client.hpp constant.hpp
	$(CXX) $(CXXFLAGS) -c client.cpp -lpthread

filelogger.o: filelogger.cpp filelogger.hpp client.hpp
	$(CXX) $(CXXFLAGS) -c filelogger.cpp -lpthread

cache.o: cache.cpp cache.hpp
	$(CXX) $(CXXFLAGS) -c cache.cpp -lpthread

response.o: response.cpp response.hpp
	$(CXX) $(CXXFLAGS) -c response.cpp -lpthread

request.o: request.cpp request.hpp
	$(CXX) $(CXXFLAGS) -c request.cpp -lpthread
.PHONY:
	clean
clean:
	rm -rf *.o main
