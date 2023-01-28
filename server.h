#pragma once

#include "clientServer.h"

/*
This function takes in three parameters: an integer new_fd, a character pointer
response, and a pointer to a function. It creates a pipe and forks the process.
In the child process, it closes the read end of the pipe, redirects stdout to
the write end of the pipe, closes the duplicate write end, calls the passed
function with new_fd and response as parameters, then exits. In the parent
process, it waits for the child process to finish executing, reads from the read
end of the pipe into a buffer of size BUFFERSIZEMAX, sends this buffer to new_fd
using send(), then closes the read end of the pipe.
*/
void pipeRedirect(int new_fd, char *response, void (*func)(int, char *)) {
  int pipefd[2];
  pipe(pipefd);

  pid_t child_pid = fork();
  if (child_pid == 0) {
    close(pipefd[0]);        // close the read end in the child process
    dup2(pipefd[1], 1);      // redirect stdout to the write end of the pipe
    close(pipefd[1]);        // close the duplicate write end
    func(new_fd, response);  // call the passed function in the child process
    _exit(0);
  } else {
    int status;
    close(pipefd[1]);  // close the write end in the parent process
    waitpid(child_pid, &status, 0);
    char buf[BUFFERSIZEMAX];
    int numbytes = read(pipefd[0], buf, BUFFERSIZEMAX);
    send(new_fd, buf, numbytes, 0);
    close(pipefd[0]);
  }
}  // End of pipeRedirect()

/*
  printHelpMenu() is a function that prints out a help menu to the user. It
  takes in an integer parameter, new_fd, which is the file descriptor for the
  connection. The help menu is stored in a constant character array and sent to
  the user using the send() function. If an error occurs while sending, an error
  message is printed using perror().
*/
void printHelpMenu(int new_fd) {
  const char helpMenu[] =
      "Help Menu:\nl: List\nc: Check <file name>\np: Display <file "
      "name>\nd: Download <file name>\nq: Quit\nh: Help\n";

  int n = send(new_fd, helpMenu, strnlen(helpMenu, BUFFERSIZEMAX), 0);
  if (n == -1) {
    perror("send");
  }
}  // End of printHelpMenu()

void getPwd(char *pwd) {
  int pipefd[2];
  pipe(pipefd);
  pid_t child_pid = fork();
  if (child_pid == 0) {
    close(pipefd[0]);    // close the read end in the child process
    dup2(pipefd[1], 1);  // redirect stdout to the write end of the pipe
    close(pipefd[1]);    // close the duplicate write end
    execlp("pwd", "pwd", NULL);
    _exit(0);
  } else {
    int status;
    close(pipefd[1]);  // close the write end in the parent process
    waitpid(child_pid, &status, 0);
    char buf[BUFFERSIZEMAX];
    int numbytes = read(pipefd[0], buf, BUFFERSIZEMAX);
    printf("contents of buf from getPwd: %s\n", buf);
    strcpy(pwd, buf);
    close(pipefd[0]);
  }
}

/*
This function takes in four char pointers as parameters: response, pwd,
filename, and fullpath. It extracts the filename from the response using sscanf
and stores it in the filename parameter. It then copies the pwd into fullpath,
concatenates a "/" character to it, and then concatenates the filename to it.
The result is a full path that is stored in the fullpath parameter.
*/
void processResponse(char *response, char *pwd, char *filename,
                     char *fullpath) {
  // extracts the filename from the response
  sscanf(response, "%*c %s", filename);

  // concatenate directory and filename for fullpath
  strcpy(fullpath, pwd);
  strcat(fullpath, "/");
  strcat(fullpath, filename);
}

/*
This function checks if a file exists in the "./sdir/" directory. It takes two
parameters, an integer representing a file descriptor and a character pointer to
a response. It extracts the filename from the response and concatenates it with
the directory name to get the full path of the file. It then checks if the
server has access to that file using the access() function. If it does, it sends
back a message saying that the file exists, otherwise it sends back a message
saying that it does not exist. Finally, it sends back a reply to the client
using the send() function.
*/
void checkForFile(int new_fd, char *response) {
  char pwd[BUFFERSIZEMAX];
  getPwd(pwd);
  char filename[BUFFERSIZEMAX];
  char fullpath[BUFFERSIZEMAX];
  processResponse(response, pwd, filename, fullpath);

  char replyMsg[BUFFERSIZEMAX] = {0};
  // check if the server has the file named <filename>
  if (access(fullpath, F_OK) != -1) {
    // file exists
    snprintf(replyMsg, BUFFERSIZEMAX + 64, "File '%s' exists", filename);
  } else {
    // file does not exist
    snprintf(replyMsg, BUFFERSIZEMAX + 64, "File '%s' not found", filename);
  }

  // Send reply back to client
  send(new_fd, replyMsg, strlen(replyMsg), 0);
}  // End of checkForFile

/*
  This function is used to display a file from the server. It takes in two
  parameters, an integer representing a file descriptor and a character pointer
  to a response. It first extracts the filename from the response and
  concatenates it with the directory name "./sdir/" to get the full path of the
  file. It then checks if the server has access to this file using the access()
  function. If it does, it executes cat on this file using execlp(). If not, it
  sends an error message back to the client indicating that the file was not
  found.
*/
void displayFile(int new_fd, char *response) {
  char pwd[BUFFERSIZEMAX];
  getPwd(pwd);
  char filename[BUFFERSIZEMAX];
  char fullpath[BUFFERSIZEMAX];
  processResponse(response, pwd, filename, fullpath);

  // check if the server has the file named <filename>
  if (access(fullpath, F_OK) != -1) {
    // file exists
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
void sendFileToClient(int new_fd, char *response) {
  char pwd[BUFFERSIZEMAX];
  getPwd(pwd);
  char filename[BUFFERSIZEMAX];
  char fullpath[BUFFERSIZEMAX];
  processResponse(response, pwd, filename, fullpath);

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

      // Determine file_size
      fseek(file, 0, SEEK_END);
      file_size = ftell(file);
      rewind(file);

      // Put file_size in network bit order before sending
      file_size = htonl(file_size);
      send(new_fd, &file_size, sizeof(file_size), 0);

      // Ferry file data accross socket to client
      while (file_size >= 0) {
        int bytes_read = fread(fileBuffer, 1, sizeof(fileBuffer), file);
        if (send(new_fd, fileBuffer, bytes_read, 0) < 0) {
          perror("Error sending file");
          break;
        }
        if (bytes_read < BUFFERSIZEMAX) {
          // End of file reached
          break;
        }
        file_size -= bytes_read;
      }
      fclose(file);
    }
  } else {
    // file does not exist
    // indicate to client that no file is coming by sending a filesize of 0
    int file_size = 0;
    // Put file_size in network bit order before sending
    file_size = htonl(file_size);
    send(new_fd, &file_size, sizeof(file_size), 0);

    // Send notification of error back to client
    char replyMsg[BUFFERSIZEMAX];
    int len =
        snprintf(replyMsg, BUFFERSIZEMAX, "File '%s' not found", filename);
    send(new_fd, replyMsg, len, 0);
  }
}  // end of sendFileToClient()

void listFile(int new_fd, char *response) {
  char pwd[BUFFERSIZEMAX];
  getPwd(pwd);
  printf("\n\npwd is: %s\n\n", pwd);
  execlp("ls", "ls", "-l", pwd, NULL);
}