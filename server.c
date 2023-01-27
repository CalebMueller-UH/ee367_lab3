/*
    File: server.c
    Description:
        a stream socket server demo
        EE367L - Lab #3 - Part 2 Assignment
    Name: Caleb Mueller
    Date: 26 January 2023
*/

#include "clientServer.h"

#define PORT "3622"  // Group assigned port used by server: used by the client
#define BACKLOG 10   // how many pending connections queue will hold

// sigchld_handler is used to clean up zombie processes
// This will be explained later
void sigchld_handler(int s) {
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

// Definition of get_in_addr held in clientServer.h
void *get_in_addr(struct sockaddr *sa);

int main(void) {
  // Get address information for this server which includes the IP address of
  // the local machine, and the TCP port number of the server, in this
  // case PORT = “3490”. It’s a stream connection which can be
  // IPv4 or IPv6. This is similar to client.c but we want the
  // IP address of the local machine
  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;  // use the IP of the local machine
  /*
    AI_PASSIVE indicates that resulting socket will be used for listening by a
    server
  */

  struct addrinfo *servinfo;  // The address results which is a linked list
  int rv;
  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // Loop through all the linked list of addressinfo results
  // and create a socket and bind it to the addressinfo.
  // Stop when the bind is successful.
  int sockfd;
  struct addrinfo *p;
  for (p = servinfo; p != NULL; p = p->ai_next) {
    // Create a socket, if possible
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;  // If it doesn’t work then skip the rest of this pass
      // and continue with loop
    }
    // setsockopt sets the socket so that it can rebind to its port after crash
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }
    // Let’s bind the socket with the socket address for ‘p’.
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;  // If we can’t bind then skip the rest of this pass
      // and continue with the loop
    }
    // We created a socket and it’s bound to a socket address with
    // the IP address of the local machine and TCP port number PORT
    // We’re done so we can break out of the loop
    break;
  }

  if (p == NULL) {  // We failed to create a socket for the server
    fprintf(stderr, "server: failed to bind\n");
    return 2;
  }
  // We now have a socket for the server for listening
  freeaddrinfo(servinfo);  // all done with this structure
  // Server starts to listen for connection requests from clients.
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }
  // In what follows, the server will create child processes. It turns out that
  // the child processes can become zombie processes. What immediately follows
  // is a method to get rid of zombie processes. This will be explained later.
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;  // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
  printf("server: waiting for connections...\n");
  // The server runs forever in a while-loop
  // In each pass of the loop, the server will accept() a connection
  // request from a client. accept() will return the socket fd for
  // the connection.
  //
  // After creating the connection, the server will create a child process
  // to reply to the client.
  while (1) {                            // main accept() loop
    struct sockaddr_storage their_addr;  // connector's addr information
    socklen_t sin_size = sizeof their_addr;
    int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    int pipefd[2];
    pipe(pipefd);

    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    // Display who the client is
    char s[INET6_ADDRSTRLEN];
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: got connection from %s\n", s);

    // If the server was able to successfully create a fork
    if (!fork()) {
      close(pipefd[0]);          // close the read end in the child process
      dup2(pipefd[1], 1);        // redirect stdout to the write end of the pipe
      close(pipefd[1]);          // close the duplicate write end
      execlp("ls", "ls", NULL);  // execute the ls command
    } else {
      close(pipefd[1]);  // close the write end in the parent process
      char buf[MAXDATASIZE];
      int numbytes = read(pipefd[0], buf, MAXDATASIZE);
      send(new_fd, buf, numbytes, 0);
      close(pipefd[0]);
      close(new_fd);
    }

    //////////////// Exercise 2 Implementation /////////////////////////////
    // Print out the received string sent by client to server
    // char receivedString[256];
    // int numBytes;
    // if ((numBytes = recv(new_fd, receivedString, sizeof(receivedString), 0))
    // ==
    //     -1) {
    //   perror("recv");
    //   exit(1);
    // }
    // receivedString[numBytes] = '\0';
    // printf("Server: received '%s'\n", receivedString);

    // // Convert the received string's lowercase characters to uppercase
    // for (int i = 0; i < numBytes; i++) {
    //   receivedString[i] = toupper(receivedString[i]);
    // }

    // // return the capitalized string back to the client
    // if (send(new_fd, receivedString, numBytes, 0) == -1) {
    //   perror("send");
    //   exit(1);
    // }
    // close(new_fd);  // parent doesn't need this
    // exit(0);
  }
  return 0;
}