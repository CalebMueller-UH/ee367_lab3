/*
    File: client.c
    Description:
        a stream socket client demo
        EE367L - Lab #3 - Part 2 Assignment
    Name: Caleb Mueller
    Date: 26 January 2023
*/

#include "clientServer.h"

#define PORT "3622"  // group assigned server port

// get sockaddr, IPv4 or IPv6 depending on the protocol
void *get_in_addr(struct sockaddr *sa);

int main(int argc, char *argv[]) {
  // Temporarily turned off for testing.  @todo, reimplement usage instructions
  //   if (argc != 2) {
  //     fprintf(stderr, "usage: client hostname\n");
  //     exit(1);
  //   }

  // Get address information of server with IP address (or domain name)
  // ‘argv[1]’ and TCP port number PORT = “3490”. It’s a stream connection which
  // can be IPv4 or IPv6

  // hints used in getaddrinfo()
  struct addrinfo hints;

  // Initialize hints to 0 to avoid problems from leftover data in memory
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;      // Can be either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;  // TCP stream socket
  struct addrinfo *servinfo;  // It will point to the results from getaddrinfo()
  int rv;
  if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  ////////////////// Connect to the server: ///////////////////////////
  // loop through all the results and connect to the first we can
  int sockfd;
  struct addrinfo *p;
  for (p = servinfo; p != NULL; p = p->ai_next) {
    // Create a socket, if possible
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;  // If it doesn’t work then skip the rest of this pass
      // and continue with loop
    }
    // We created a socket, now let’s establish a connection to the server
    // with the socket address for ‘p’.
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("client: connect");
      continue;  // If we can’t establish a connection then skip
      // the rest of this pass and continue with loop
    }
    // We created a socket and established a connection so we’re done
    // with the loop
    break;
  }
  if (p == NULL) {  // We failed to connect after going through
    // the linked list servinfo
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }
  // We established a connection with the server
  // Notify the user that we made the connection
  char s[INET6_ADDRSTRLEN];  // Buffer to store char string
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s,
            sizeof s);
  printf("client: connecting to %s\n", s);
  freeaddrinfo(servinfo);  // all done with this structure

  ///////////////// Exercise 2 Implementation //////////////////////////////
  ///////////////// Send argv[] contents to server socket //////////////////
  /*
    When calling client from the command line argv[1] is the hostname of the
    server. Start sending the contents of argv[2] for as many elements that
    exist If argc < 2 then do nothing
  */
  char sendbuf[MAXDATASIZE];
  memset(sendbuf, 0, sizeof(sendbuf));
  for (int i = 2; i < argc; i++) {
    strcat(sendbuf, argv[i]);
    strcat(sendbuf, " ");
  }
  sendbuf[strlen(sendbuf) - 1] = '\0';

  // Send the command line arguments to the server
  if (send(sockfd, sendbuf, strlen(sendbuf), 0) == -1) {
    perror("send");
    exit(1);
  }

  // Display what’s received from server
  int numbytes;
  char buf[MAXDATASIZE];
  if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
    // Note that recv() can be replaced by read() since flags = 0
    perror("recv");
    exit(1);
  }
  buf[numbytes] = '\0';
  printf("client: received '%s'\n", buf);
  close(sockfd);
  return 0;
}