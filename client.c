#include "clientServer.h"

#define PORT "3622"  // group assigned server port

// get sockaddr, IPv4 or IPv6 depending on the protocol
void *get_in_addr(struct sockaddr *sa);

// function for freeing correct socket shutdown
void closeSocket(struct addrinfo *servinfo, int sockfd);

int main(int argc, char *argv[]) {
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

  int sockfd;

  while (1) {
    ////////////////// Connect to the server: ///////////////////////////
    struct addrinfo *p;
    for (p = servinfo; p != NULL; p = p->ai_next) {
      // Attempt Socket Creation
      if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
          -1) {
        // Socket Creation Failed
        perror("client: socket");
        continue;
      }
      // Socket Creation Success
      // Attempt to Connect Socket
      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        // Connection Failed
        close(sockfd);
        perror("client: connect");
        continue;
      }
      break;
    }
    if (p == NULL) {
      // failed to connect after going through linked list
      fprintf(stderr, "client: failed to connect\n");
      return 2;
    }

    // Socket Connection Passed ****************************
    // Notify user of connection success
    char s[INET6_ADDRSTRLEN];  // Buffer to store char string
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s,
              sizeof s);
    // printf("client: connecting to %s\n", s);

    // Display what’s received from server
    int numbytes;
    char buf[BUFFERSIZE];
    if ((numbytes = recv(sockfd, buf, BUFFERSIZE - 1, 0)) == -1) {
      perror("recv");
      exit(1);
    }
    buf[numbytes] = '\0';
    printf("%s", buf);

    // Get input from the user
    char input[BUFFERSIZE];
    fgets(input, BUFFERSIZE, stdin);

    // Check if user wants to quit
    if (input[0] == 'q') {
      closeSocket(servinfo, sockfd);
      break;
    }

    // Send input to the server
    if (send(sockfd, input, strlen(input), 0) == -1) {
      perror("send");
      exit(1);
    }

    // Receive response from the server
    if ((numbytes = recv(sockfd, buf, BUFFERSIZE - 1, 0)) == -1) {
      perror("recv");
      exit(1);
    }
    buf[numbytes] = '\0';
    printf("%s", buf);

  }  // End of while(1)
  return 0;
}  // End of main()

// function for freeing correct socket shutdown
void closeSocket(struct addrinfo *servinfo, int sockfd) {
  freeaddrinfo(servinfo);  // all done with this structure
  close(sockfd);
}
