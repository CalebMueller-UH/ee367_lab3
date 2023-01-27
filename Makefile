CC = gcc
CFLAGS = -c -Wall

all: server client

all:
	make server
	make client

server:
	$(CC) -o server server.c clientServer.h

client:
	$(CC) -o client client.c clientServer.h

clean:
	rm -f server client
