CC=g++
CFLAGS=-I. -std=c++11

all: server main ss awget

server:
	$(CC) $(CFLAGS) -c ss.cc
	
main:
	$(CC) $(CFLAGS) -c main.cc
	
ss: ss.o  main.o
	$(CC) $(CFLAGS) -o ss ss.o main.o -lpthread

awget:
	$(CC) $(CFLAGS) -o awget awget.cc
	
clean:
	rm *.o awget ss
