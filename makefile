#Makefile
CC = g++
INCLUDE = /usr/lib
LIBS = -lcrypto -lssl
OBJS =
CFLAGS = -g
all: proxy

proxy:
	$(CC) -o bin/webproxy -Iheaders headers/*.cpp webproxy.cpp $(CFLAGS) $(LIBS)

clean:
	rm -rf bin
	mkdir -p bin
