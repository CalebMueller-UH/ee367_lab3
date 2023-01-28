CC=gcc
CFLAGS= -Wall

all: clean server client

server:
	$(CC) $(CFLAGS) -o ./sdir/server ./server.c ./clientServer.h

client: 
	$(CC) $(CFLAGS) -o ./cdir/client client.c clientServer.h 
	
clean: 
	rm -f ./sdir/server ./cdir/client 
