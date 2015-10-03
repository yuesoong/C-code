CC = gcc
CFLAGS = -Wall -pedantic -ggdb -DDEBUG -lpthread

all: bridge bridgeOne

bridge: bridge.o
	$(CC) $(CFLAGS) -o bridge bridge.o

bridge.o: bridge.c
	$(CC) $(CFLAGS) -c bridge.c

bridgeOne: bridgeOne.o
	$(CC) $(CFLAGS) -o bridgeOne bridgeOne.o

bridgeOne.o: bridgeOne.c
	$(CC) $(CFLAGS) -c bridgeOne.c

clean:
	rm -rf *~
	rm -rf *#
	rm -rf *.o
