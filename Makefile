CXX = g++
CXXFLAGS = -g

all: main

main: main.o proxy.o client.o
	$(CXX) $(CXXFLAGS) -o main main.o proxy.o client.o -lpthread

main.o: main.cpp main.hpp proxy.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

proxy.o: proxy.cpp proxy.hpp constant.hpp client.hpp
	$(CXX) $(CXXFLAGS) -c proxy.cpp -lpthread

client.o: client.cpp client.hpp constant.hpp
	$(CXX) $(CXXFLAGS) -c client.cpp -lpthread

.PHONY:
	clean
clean:
	rm -rf *.o main
