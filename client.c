#include "clientServer.h"

#define PORT "3622"  // group assigned server port

// get sockaddr, IPv4 or IPv6 depending on the protocol
void *get_in_addr(struct sockaddr *sa);

// function for freeing correct socket shutdown
void closeSocket(struct addrinfo *servinfo, int sockfd) {
  freeaddrinfo(servinfo);  // all done with this structure
  close(sockfd);
}

void initiateFileDownload(int sockfd, char *input) {
  // Grab filename from command "d <filename>"
  char filename[BUFFERSIZEMAX];
  sscanf(input, "%*c %s", filename);
  // create a char array to store the full path of the file
  char fullpath[BUFFERSIZEMAX] = "./cdir/";
  strcat(fullpath, filename);  // concatenate the filename to the path

  // Determine if the filename already exists
  if (access(fullpath, F_OK) != -1) {
    // filename already exists, query user if they'd like to overwrite
    printf("The file %s already exists, do you want to overwrite it? (y/n)",
           fullpath);
    char answer[BUFFERSIZEMAX];
    fgets(answer, BUFFERSIZEMAX, stdin);
    if (answer[0] == 'y' || answer[0] == 'Y') {
      // User wants to overwrite
      // Continue without returning
    } else {
      // User doesn't want to overwrite
      printf("n happened\n");
      return;
    }
  }

  // filename doesn't exists or user wants to overwrite
  // Download the file
  // Send download command to server to initiate file download
  if (send(sockfd, input, strnlen(input, BUFFERSIZEMAX), 0) == -1) {
    perror("initializeFileDownload: send error");
    exit(1);
  }

  // Open a file to write the downloaded data
  FILE *fp;
  fp = fopen(fullpath, "w+");
  if (fp == NULL) {
    perror("initializeFileDownload: Error opening file");
    exit(1);
  }

  // Receive the file data from the server
  char buffer[BUFFERSIZEMAX];
  int bytesBeingRead;

  // Get the file_size from the server
  int file_size;
  if (recv(sockfd, &file_size, sizeof(file_size), 0) < 0) {
    perror("initializeFileDownload: filesize receipt error");
    exit(1);
  }
  file_size = ntohl(file_size);

  // char file_size_str[BUFFERSIZEMAX];
  // if ((bytesBeingRead = recv(sockfd, file_size_str, BUFFERSIZEMAX - 1, 0)) >
  //     0) {
  //   int file_size = atoi(file_size_str);
  // } else {
  //   perror("initializeFileDownload: filesize receipt error");
  //   exit(1);
  // }

  printf("file_size: %d\n", file_size);

  int recData = 0;
  while ((bytesBeingRead = recv(sockfd, buffer, BUFFERSIZEMAX - 1, 0)) > 0) {
    recData += bytesBeingRead;
    printf("recData: %d, bytesBeingRead: %d\n", recData, bytesBeingRead);
    fwrite(buffer, sizeof(char), bytesBeingRead, fp);
    printf("wrote to file\n");
    if (bytesBeingRead < BUFFERSIZEMAX || recData >= file_size) {
      // End of file reached
      break;
    }
  }

  // Close the file when done writing
  if (fclose(fp) != 0) {
    perror("initializeFileDownload: error closing file");
  }
  printf("file closed\n");
}  // end of initiateFileDownload()

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
    char buf[BUFFERSIZEMAX];
    if ((numbytes = recv(sockfd, buf, BUFFERSIZEMAX - 1, 0)) == -1) {
      perror("recv");
      exit(1);
    }
    buf[numbytes] = '\0';
    printf("%s", buf);

    // Get input from the user
    char input[BUFFERSIZEMAX];
    fgets(input, BUFFERSIZEMAX, stdin);

    // Check if user wants to quit
    if (input[0] == 'q' || input[0] == 'Q') {
      closeSocket(servinfo, sockfd);
      break;
    } else if (input[0] == 'd' || input[0] == 'D') {
      initiateFileDownload(sockfd, input);
      continue;
    }

    // Send input to the server
    if (send(sockfd, input, strlen(input), 0) == -1) {
      perror("send");
      exit(1);
    }

    // Receive response from the server
    if ((numbytes = recv(sockfd, buf, BUFFERSIZEMAX - 1, 0)) == -1) {
      perror("recv");
      exit(1);
    }
    buf[numbytes] = '\0';
    printf("%s", buf);

  }  // End of while(1)
  return 0;
}  // End of main()
