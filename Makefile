CC = g++
CFLAGS = -g

all: main

main: main.o proxy.o client.o
	$(CC) $(CFLAGS) -o main main.o proxy.o client.o

main.o: main.cpp main.hpp proxy.hpp
	$(CC) $(CFLAGS) -c main.cpp

proxy.o: proxy.cpp proxy.hpp constant.hpp client.hpp
	$(CC) $(CFLAGS) -c proxy.cpp

client.o: client.cpp client.hpp
	$(CC) $(CFLAGS) -c client.cpp

.PHONY:
	clean
clean:
	rm -rf *.o main
