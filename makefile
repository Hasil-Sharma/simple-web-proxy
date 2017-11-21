#Makefile
CC = g++
INCLUDE = /usr/lib
LIBS =
OBJS =
CFLAGS = -g
PORT = 8001
TIMEOUT = 10
all: clean proxy run

proxy:
	$(CC) -o bin/webproxy -Iheaders headers/*.cpp webproxy.cpp $(CFLAGS) $(LIBS)

clean:
	rm -rf bin
	mkdir -p bin

run:
	bin/webproxy $(PORT) $(TIMEOUT) &> logs/webproxy.log &
	tail -f logs/webproxy.log

log:
	tail -f logs/webproxy.log
kill:
	fuser -k $(PORT)/tcp
