CC = gcc
CFLAGS = -c -Wall

all: server367 client367

all:
	make server367
	make client367

server367:
	$(CC) -o server367 server.c clientServer.h

client367:
	$(CC) -o client367 client.c clientServer.h

clean:
	rm -f server367 client367
