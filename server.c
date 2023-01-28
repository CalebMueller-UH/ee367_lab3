#include "clientServer.h"

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

void printHelpMenu(int new_fd) {
  const char helpMenu[] =
      "Help Menu:\nl: List\nc: Check <file name>\np: Display <file "
      "name>\nd: Download <file name>\nq: Quit\nh: Help\n";

  int n = send(new_fd, helpMenu, strnlen(helpMenu, BUFFERSIZEMAX), 0);
  if (n == -1) {
    perror("send");
  }
}  // End of printHelpMenu()

void checkForFile(int new_fd, char *response) {
  // extracts the filename from the response
  char filename[BUFFERSIZEMAX] = {0};
  sscanf(response, "%*c %s", filename);

  char replyMsg[BUFFERSIZEMAX] = {0};
  // check if the server has the file named <filename>
  if (access(filename, F_OK) != -1) {
    // file exists
    snprintf(replyMsg, BUFFERSIZEMAX + 64, "File '%s' exists", filename);
  } else {
    // file does not exist
    snprintf(replyMsg, BUFFERSIZEMAX + 64, "File '%s' not found", filename);
  }

  // Send reply back to client
  send(new_fd, replyMsg, strlen(replyMsg), 0);
}  // End of checkForFile

void displayFile(int new_fd, char *response, int pipefd[]) {
  // extracts the filename from the response
  char filename[BUFFERSIZEMAX];
  sscanf(response, "%*c %s", filename);

  // check if the server has the file named <filename>
  char fullpath[BUFFERSIZEMAX] =
      "./sdir/";  // create a char array to store the full path of the file
  strcat(fullpath, filename);  // concatenate the filename to the path
  if (access(fullpath, F_OK) != -1) {
    // file exists
    // setup a pipe to redirect server system stdout to client stream
    close(pipefd[0]);    // close the read end in the child process
    dup2(pipefd[1], 1);  // redirect stdout to the write end of the pipe
    close(pipefd[1]);    // close the duplicate write end
    // execute the cat command
    execlp("cat", "cat", fullpath, NULL);
  } else {
    // file does not exist
    // Send notification of error back to client
    char replyMsg[BUFFERSIZEMAX];
    int len =
        snprintf(replyMsg, BUFFERSIZEMAX, "File '%s' not found", filename);
    send(new_fd, replyMsg, len, 0);
  }
}  // End of displayFile()

/*
  This function downloads a file from the server to the client. It takes two
parameters, an integer new_fd representing the file descriptor of the client,
and a char pointer response containing the request from the client.

The function first extracts the filename from the response using sscanf(). It
then checks if the server has a file with that name using access(). If it does,
it sends the file to the client by opening it in read binary mode and sending it
in chunks of BUFFERSIZEMAX bytes. If not, it sends an error message back to the
client. Finally, if a file was opened, it is closed before returning.
*/
void downloadFile(int new_fd, char *response) {
  // extracts the filename from the response
  char filename[BUFFERSIZEMAX];
  sscanf(response, "%*c %s", filename);
  // create a char array to store the full path of the file
  char fullpath[BUFFERSIZEMAX] = "./sdir/";
  strcat(fullpath, filename);  // concatenate the filename to the path

  // check if the server has the file named <filename>
  if (access(fullpath, F_OK) != -1) {
    // file exists
    // send the file to the client
    int file_size;
    char fileBuffer[BUFFERSIZEMAX];
    FILE *file = fopen(fullpath, "rb");

    // Check if file was successfully opened
    if (file == NULL) {
      perror("Error opening file");
    } else {
      // File opened successfully

      // Determine file_ize
      fseek(file, 0, SEEK_END);
      file_size = ftell(file);
      rewind(file);

      // Put file_size in network bit order before sending
      file_size = htonl(file_size);
      send(new_fd, &file_size, sizeof(file_size), 0);

      // Send file_size to client
      // memset(&fileBuffer, 0, sizeof fileBuffer);
      // snprintf(fileBuffer, sizeof(fileBuffer), "%d", file_size);
      // send(new_fd, fileBuffer, strnlen(fileBuffer, BUFFERSIZEMAX), 0);

      // Ferry file data accross socket to client
      int sentData = 0;
      while (file_size > 0) {
        int bytes_read = fread(fileBuffer, 1, sizeof(fileBuffer), file);
        sentData += bytes_read;
        printf("Sent data: %d\n", sentData);
        if (send(new_fd, fileBuffer, bytes_read, 0) < 0) {
          perror("Error sending file");
          break;
        }
        if (bytes_read < BUFFERSIZEMAX) {
          printf("End of file reached\n");
          // End of file reached
          break;
        }
        file_size -= bytes_read;
      }
      fclose(file);
    }
  } else {
    // file does not exist
    // Send notification of error back to client
    char replyMsg[BUFFERSIZEMAX];
    snprintf(replyMsg, strnlen(replyMsg, BUFFERSIZEMAX), "File '%s' not found",
             filename);
    send(new_fd, replyMsg, strnlen(replyMsg, BUFFERSIZEMAX), 0);
  }
  printf("reached end of downloadFile\n");
}  // end of downloadFile()

int main(void) {
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

    // Set up a pipe for stdout redirection via dup2 for server system output
    int pipefd[2];
    pipe(pipefd);

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
      int n = send(new_fd, prompt, strnlen(prompt, BUFFERSIZEMAX), 0);
      if (n == -1) {
        perror("send");
        continue;
      }

      // Wait to receive a response back from the client
      char response[256];
      n = recv(new_fd, response, sizeof(response), 0);
      if (n == -1) {
        perror("recv");
        continue;
      }

      // Print the response received from the client
      printf("server: received response from %s: %s\n", s, response);

      switch (response[0]) {
        case 'l':
        case 'L':
          // setup a pipe to redirect server system stdout to client stream
          close(pipefd[0]);    // close the read end in the child process
          dup2(pipefd[1], 1);  // redirect stdout to the write end of the pipe
          close(pipefd[1]);    // close the duplicate write end
                               // execute the ls command
          execlp("ls", "ls", "-l", "./sdir", NULL);
          break;
        case 'c':
        case 'C':
          checkForFile(new_fd, response);
          break;
        case 'd':
        case 'D':
          downloadFile(new_fd, response);
          break;
        case 'h':
        case 'H':
          printHelpMenu(new_fd);
          break;
        case 'p':
        case 'P':
          displayFile(new_fd, response, pipefd);
          break;
        default:
          char blank[] = "";
          int n = send(new_fd, blank, strnlen(blank, BUFFERSIZEMAX), 0);
          if (n == -1) {
            perror("send");
            continue;
          }
      }  // end of switch

    } else {
      close(pipefd[1]);  // close the write end in the parent process
      char buf[BUFFERSIZEMAX];
      int numbytes = read(pipefd[0], buf, BUFFERSIZEMAX);
      send(new_fd, buf, strnlen(buf, BUFFERSIZEMAX), 0);
      close(pipefd[0]);
      close(new_fd);
    }
  }
  return 0;
}