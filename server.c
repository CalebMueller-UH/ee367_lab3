/*
    File: server.c
    Description:
        a stream socket client demo
        EE367L - Lab #3 - Part 2 Assignment
    Name: Caleb Mueller
    Date: 28 January 2023
*/

#include "server.h"

#define PORT "3622"  // Group assigned port used by server: used by the client
#define BACKLOG 10   // how many pending connections queue will hold

// sigchld_handler is used to clean up zombie processes
// This will be explained later
void sigchld_handler(int s) {
  while (waitpid(-1, NULL, WNOHANG) > 0) {
    continue;  // do nothing
  }
}

// Definition of get_in_addr found in clientServer.h
void *get_in_addr(struct sockaddr *sa);

int main(void) {
  // Use the ./sdir directory which contains the correct server files
  char *path = "./sdir/";
  if (chdir(path) != 0) {
    perror("chdir failed");
    return 1;
  }

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

  // The server runs forever in a while-loop ****************************
  // In each pass of the loop, the server will accept() a connection
  // request from a client. accept() will return the socket fd for
  // the connection.
  //
  // After creating the connection, the server will create a child process
  // to reply to the client.
  while (1) {
    // main accept() loop
    struct sockaddr_storage their_addr;  // connector's addr information
    socklen_t sin_size = sizeof their_addr;

    // Accept connection from a client
    int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    // Print the IP address of the client that connected to the server
    char s[INET6_ADDRSTRLEN];
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: connection from %s\n", s);

    // If the server was able to successfully create a fork
    if (!fork()) {
      // Send command prompt
      const char prompt[] = "\nCommand(enter 'h' for help): ";
      int n = send(new_fd, prompt, sizeof(prompt), 0);
      if (n == -1) {
        perror("send");
        continue;
      }

      // Wait to receive a response back from the client
      char response[BUFFERSIZEMAX];
      n = recv(new_fd, response, sizeof(response), 0);
      if (n == -1) {
        perror("recv");
        continue;
      }

      // Print the response received from the client
      printf("server: received response from %s: %s\n", s, response);

      switch (response[0]) {
        break;
        case 'c':
        case 'C':
          checkForFile(new_fd, response);
          break;
        case 'd':
        case 'D':
          pipeRedirect(new_fd, response, sendFileToClient);
          break;
        case 'h':
        case 'H':
          printHelpMenu(new_fd);
          break;
        case 'l':
        case 'L':
          pipeRedirect(new_fd, response, listFile);
          break;
        case 'p':
        case 'P':
          pipeRedirect(new_fd, response, sendFileToClient);
          break;
        default:
          char blank[] = "";
          int n = send(new_fd, blank, sizeof(blank), 0);
          if (n == -1) {
            perror("send");
            continue;
          }
      }  // end of switch

    } else {
      close(new_fd);
    }
  }
  return 0;
}
