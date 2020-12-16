#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "util.h"


#define BACKLOG 20
#define GET_REQ_SIZE 2048
#define LINE_LENGTH 128

int sockfd;
/**********************************************
 * init
   - port is the number of the port you want the server to be
     started on
   - initializes the connection acception/handling system
   - YOU MUST CALL THIS EXACTLY ONCE (not once per thread,
     but exactly one time, in the main thread of your program)
     BEFORE USING ANY OF THE FUNCTIONS BELOW
   - if init encounters any errors, it will call exit().
************************************************/
void init(int port) {
  if( (sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1 ) {
    perror("Failed to make socket");
    exit(0);
  }

  int enable = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*) &enable, sizeof(int));

  struct sockaddr_in addr; 
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  if(bind(sockfd, (struct sockaddr*) &addr, sizeof(addr)) == -1){
    perror("Failed to bind socket");
    exit(0);
  }

  if(listen(sockfd, BACKLOG) == -1){
    perror("Failed to listen");
    exit(0);
  }

}

/**********************************************
 * accept_connection - takes no parameters
   - returns a file descriptor for further request processing.
     DO NOT use the file descriptor on your own -- use
     get_request() instead.
   - if the return value is negative, the request should be ignored.
***********************************************/
int accept_connection(void) {
  int fd; 
  if((fd = accept(sockfd, NULL, NULL)) < 0){
    perror("Failed to accept connection");
  } 
  return fd;
}

/**********************************************
 * get_request
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        from where you wish to get a request
      - filename is the location of a character buffer in which
        this function should store the requested filename. (Buffer
        should be of size 1024 bytes.)
   - returns 0 on success, nonzero on failure. You must account
     for failures because some connections might send faulty
     requests. This is a recoverable error - you must not exit
     inside the thread that called get_request. After an error, you
     must NOT use a return_request or return_error function for that
     specific 'connection'.
************************************************/
int get_request(int fd, char *filename) {
  char buf[GET_REQ_SIZE];
  if(read(fd, buf, GET_REQ_SIZE) == -1){
    perror("Failed to read");
    return 1;
  }

  char request_type[100]; 
  sscanf(buf, "%s %s", request_type, filename);
  printf("Request Type: %s filename: %s\n", request_type, filename);

  if(strcmp("GET", request_type)){
    fprintf(stderr, "Not a GET request\n");
    return 1;
  }
  if(!strcmp("", filename) || (strstr(filename, "..") != NULL) || (strstr(filename, "//") != NULL)){
    fprintf(stderr, "Invalid filename\n");
    return 1;
  } 
  return 0;
}

/**********************************************
 * return_result
   - returns the contents of a file to the requesting client
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        to where you wish to return the result of a request
      - content_type is a pointer to a string that indicates the
        type of content being returned. possible types include
        "text/html", "text/plain", "image/gif", "image/jpeg" cor-
        responding to .html, .txt, .gif, .jpg files.
      - buf is a pointer to a memory location where the requested
        file has been read into memory (the heap). return_result
        will use this memory location to return the result to the
        user. (remember to use -D_REENTRANT for CFLAGS.) you may
        safely deallocate the memory after the call to
        return_result (if it will not be cached).
      - numbytes is the number of bytes the file takes up in buf
   - returns 0 on success, nonzero on failure.
************************************************/
int return_result(int fd, char *content_type, char *buf, int numbytes) {
  char* header_line = "HTTP/1.1 200 OK\n";
  if(write(fd, header_line, strlen(header_line)) == -1){
    perror("Write failed");
    return 1;
  }

  char content_line [LINE_LENGTH];
  sprintf(content_line, "Content-Type: %s\n", content_type);
  if(write(fd, content_line, strlen(content_line)) == -1){
    perror("Write failed");
    return 1;
  }

  char length_line [LINE_LENGTH];
  sprintf(length_line, "Content-Length: %d\n", numbytes);
  if(write(fd, length_line, strlen(length_line)) == -1){
    perror("Write failed");
    return 1;
  }

  char* connection_line = "Connection: Close\n\n";
  if(write(fd, connection_line, strlen(connection_line)) == -1){
    perror("Write failed");
    return 1;
  }

  if(write(fd, buf, numbytes) == -1){
    perror("Write failed");
    return 1;
  }
  close(fd);
  return 0;
}

/**********************************************
 * return_error
   - returns an error message in response to a bad request
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        to where you wish to return the error
      - buf is a pointer to the location of the error text
   - returns 0 on success, nonzero on failure.
************************************************/
int return_error(int fd, char *buf) {
  char* header_line = "HTTP/1.1 404 Not Found\n";
  if(write(fd, header_line, strlen(header_line)) == -1){
    perror("Write failed");
    return 1;
  }

  char content_line [LINE_LENGTH] = "Content-Type: text/html\n";
  if(write(fd, content_line, strlen(content_line)) == -1){
    perror("Write failed");
    return 1;
  }

  char length_line [LINE_LENGTH];
  sprintf(length_line, "Content-Length: %ld\n", strlen(buf));
  if(write(fd, length_line, strlen(length_line)) == -1){
    perror("Write failed");
    return 1;
  }

  char* connection_line = "Connection: Close\n\n";
  if(write(fd, connection_line, strlen(connection_line)) == -1){
    perror("Write failed");
    return 1;
  }

  if(write(fd, buf, strlen(buf)) == -1){
    perror("Write failed");
    return 1;
  }
  close(fd);
  return 0;
}
