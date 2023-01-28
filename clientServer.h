/*
    File: clientServer.h
    Description:
        a stream socket client demo
        EE367L - Lab #3 - Part 2 Assignment
    Name: Caleb Mueller
    Date: 26 January 2023
*/

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFERSIZEMAX 256
#define MAXFILELEN 128

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {  // Return IPv4 address
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);  // Return IPv6 address
}
