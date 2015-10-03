CC = gcc
CFLAGS = -Wall -pedantic -std=c99 -ggdb -DDEBUG

all: album albumOneLoop

album: album.o
	$(CC) $(CFLAGS) -o album album.o

album.o: album.c
	$(CC) $(CFLAGS) -c album.c

albumOneLoop: albumOneLoop.o
	$(CC) $(CFLAGS) -o albumOneLoop albumOneLoop.o

albumOneLoop.o: albumOneLoop.c
	$(CC) $(CFLAGS) -c albumOneLoop.c

clean:
	rm -f $(OBJS) $(OUTPUT)
	rm -rf `find -name "*_*.jpg"`
	rm -rf *~
	rm -rf *#
	rm -rf *.o
