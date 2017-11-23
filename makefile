#Makefile
CC = g++
INCLUDE = /usr/lib
LIBS =-pthread
OBJS =
CFLAGS =-std=c++0x -g -Wall 
PORT = 8001
TIMEOUT = 10
all: clean proxy run
valgrind: clean proxy memory-leak
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

debug:
	gdb -tui bin/webproxy --args bin/webproxy $(PORT) $(TIMEOUT) 2> logs/debug.log

test:
	sh testing.sh

memory-leak:
	valgrind --leak-check=full --track-origins=yes --log-file="log.out" bin/webproxy $(PORT) $(TIMEOUT)

