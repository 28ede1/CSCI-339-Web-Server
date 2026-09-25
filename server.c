/* 
* echoserver.c - A simple connection-based echo server 
* added support for multiple threads
* usage: echoserver <port>
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <pthread.h>

#define BUFSIZE 1024

typedef struct {
char method[4]; // method length, only GET will be supported, so use 4 (GET + "\0")
char path[PATH_MAX]; // length of longest path of whatever machine the code compiles on
char version[9]; // version length, HTTP/1.1 or HTTP/1.0 (version + "\0")
} request_params;

int parseString(char *str, request_params *result);

int parseString(char *str, request_params *result) {

  char delimiter[] = " \n";

  char *portion1 = strtok(str, delimiter);

  char *portion2 = strtok(NULL, delimiter);

  char *portion3 = strtok(NULL, delimiter);

  char *portion4 = strtok(NULL, delimiter);

  if ((portion1 == NULL) || (portion2 == NULL) || (portion3 == NULL)) {
    printf("Invalid request. Too few request parameters.\n");
    return 1;
  }

  if ((portion4 != NULL)) {
    printf("Invalid request. Too many request parameters.\n");
    return 1;
  }

  if ((strcmp(portion1, "GET") != 0) || ((strcmp(portion3, "HTTP/1.1") != 0) && (strcmp(portion3, "HTTP/1.0") != 0) )){
    printf("Invalid request. Incorrect request parameters.\n");
    return 1;
  }

  strcpy(result->method, portion1);
  strcpy(result->path, portion2);
  strcpy(result->version, portion3);
  return 0;
}

void *run_thread(void *vargp);

int main(int argc, char **argv) {
  int listenfd;        /* listening socket */
  int *connfd;         /* connection socket */
  int portno;          /* port to listen on */
  socklen_t clientlen; /* byte size of client's address */
  pthread_t tid;       /* thread id */
  int optval;

  struct sockaddr_in myaddr;  /* my ip address info */
  struct sockaddr clientaddr; /* client's info */

  /* check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* first, set necessary fields in myaddr struct */
  myaddr.sin_port = htons(portno);
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
  /* make a socket for listening */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
    printf("ERROR opening socket\n");
    exit(1);
  }

  /* setsockopt: Optional but handy debugging trick that lets 
  * us rerun the server on same port immediately after we kill it; 
  * otherwise we have to wait about 20 secs. 
  * Eliminates "ERROR on binding: Address already in use" error. 
  */
  optval = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  /* bind: associate the listening socket (listenfd) with a specific IP address */
  /* in this case we'll use myaddr since we already configured it */
  if (bind(listenfd, (struct sockaddr*) &myaddr, sizeof(myaddr)) <0) {
    printf("ERROR on binding\n");
    exit(1);
  }

  /* listen: make it a listening socket ready to accept connection requests */
  if (listen(listenfd, 10) < 0) {
    printf("ERROR on listen\n");
    exit(1);
  }

  /* main loop: wait for a connection request, echo input line, 
    then close connection. */
  while (1) {

    /* accept: wait for a connection request */
    clientlen = sizeof(clientaddr);

    /* reserve space for connfd on heap, this is thread-safe! */
    connfd = malloc(sizeof(int));  
    *connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

    if (*connfd < 0) {
      printf("ERROR on accept\n");
      exit(1);
    }

    printf("Client connected!\n");

    /* try to create thread */
    /* if successful, new thread will run function "run_thread" */
    if (pthread_create(&tid, NULL, run_thread, connfd) !=0) {
      printf("error creating threads\n");
      exit(1);
    }

  }
  return 0;
}

/* new threads will start execution in this function */
void *run_thread(void *vargp) {
  char buf[BUFSIZE];             /* message buffer */
  int num_read;                  /* num bytes read */
  // int num_sent;                  /* num bytes sent */
  int connfd = *((int *)vargp);  /* nasty pointer casting. this is our client fd */
  request_params result; /* to parse and store client request */

  result.method[0] = '\0';
  result.path[0] = '\0';
  result.version[0] = '\0';

  /* detach this thread from parent thread */
  if (pthread_detach(pthread_self()) != 0){
    printf("error detaching\n");
    exit(1);
  }    

  /* free heap space for vargp since we have connfd */
  free(vargp); 

  /* recv: read input string from the client */
  bzero(buf, BUFSIZE);
  num_read = recv(connfd, buf, BUFSIZE, 0);
  if (num_read < 0) {
    printf("ERROR reading from socket\n");
    exit(1);
  }
  printf("server received %d bytes: %s\n", num_read, buf);

  /* parse string to ensure request format is good */
  int parseStringResult = parseString(buf, &result);
  if (parseStringResult) {
    printf("Invalid request\n");
  }

  /* find file contexts and check that file exists, can be read, and store file contents in some way*/

  /* format string response to send to the client, since send only sends a single string */

  /* send: echo the input string back to the client */
  num_sent = send(connfd, buf, num_read, 0);
  if (num_sent < 0)  {
    printf("ERROR writing to socket\n");
    exit(1);
  }

  /* close client */
  shutdown(connfd, 0);
  close(connfd);
  return NULL;
}