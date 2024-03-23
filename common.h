#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <pthread.h>

#ifndef common_vars
#define PORT_NUMBER 8080 
#define SA struct sockaddr
#define MAXLINE 4056
#endif
