/* 
* server.c - Listens for GET  client request and sends a response
to each client. <port>
*/

#include <stdio.h> // standard input output libraryies
#include <unistd.h> // unix OS system call functions
#include <stdlib.h> // utilities for allocating memory, converting types, and exiting
#include <string.h> // string functions like strlen(), strcmp(), memcpy()
#include <netdb.h> // looking up network addresses
#include <sys/types.h> // types used for OS functions like ssize_t for byte counts
#include <sys/socket.h> // for creating and using sockets
#include <netinet/in.h> // internet address structs and constants
#include <arpa/inet.h> // converting IP addresses between readable text and binary
#include <sys/wait.h> // functions for waiting for a child process to
#include <limits.h> // constants for limits on int values and other quantities
#include <pthread.h> // for creating and managing threads
#include <errno.h> // for accesssing error codes to explain why OS calls failed
#include <fcntl.h> // opening files and controlling access
#include <time.h> // working with dates and times
#include <sys/stat.h> // getting file information like size, permissions, etc
#include <time.h> // for time operations

#define BUFSIZE 1024

/* 
  request_params struct allows for easier storing and accessing
  of client request information
*/

typedef struct {
char method[4]; // method length, only GET will be supported, so use size 4 (GET + "\0")
char path[PATH_MAX]; // length of longest path of whatever machine the code compiles on
char version[9]; // version length, HTTP/1.1 or HTTP/1.0 (version + "\0")
} request_params;

/*
  parseString ensures parses input (str) to ensure correct format,
  and uses a pointer to a request_params struct to store request parameters 
  if successful. Returns int representing the success or failure of parsing.

  str is meant to be client input,
  result is pointer that will store what is parsed
*/

int parseString(char *str, request_params *result);

int parseString(char *str, request_params *result) {

  char delimiter[] = " \n";

  // \r means move the cursor to the beginning of the current line.
  // \r\n means move the cursor to the beginning of the current line then go to the next line
  // in most cases \n alone will move the cursor to the next line in the leftmost position, 
  // but sometimes it wont, so \r\n will explicitly do that.
  
  // strchr tries to findd the memory address of the first \r
  // if it is found, at that memory address place a terminal character instead. 
  char *line_end = strchr(str, '\r');
  if (line_end != NULL) {
      *line_end = '\0';
  }

  // strtok splits a string into pieces using separator characters.
  // separators are essentially replaced with '\0' before storing a section.
  // since we know the client requests includes spaces we can use this function
  // to help extract the parmameters. Null is returned if there are no more pieces split by 
  // the delimiters.

  char *portion1 = strtok(str, delimiter);

  char *portion2 = strtok(NULL, delimiter); // use NULL for successive calls.

  char *portion3 = strtok(NULL, delimiter);

  char *portion4 = strtok(NULL, delimiter);

  // Request line should have no less than 3 parts
  if ((portion1 == NULL) || (portion2 == NULL) || (portion3 == NULL)) {
    return 1;
  }

  // Request line should not have more than 3 parts
  if ((portion4 != NULL)) {
    return 1;
  }

  if ((strcmp(portion1, "GET") != 0) || ((strcmp(portion3, "HTTP/1.1") != 0) && (strcmp(portion3, "HTTP/1.0") != 0) )){
    return 1;
  }

  strcpy(result->method, portion1);
  strcpy(result->path, portion2);
  strcpy(result->version, portion3);
  return 0;
}

/* 
read_file attempts to open the request file 
and directly send a HTTP response to the comment.
Contains handling for 404 and 403 errors.

connfd is socket used to communicate with the connected client
path is the requested file path.

*/
void read_file(int connfd, const char *path);

void read_file(int connfd, const char *path){

  // store the opened file
	int file_fd = open(path, O_RDONLY);

  // date calculaton
  time_t now = time(NULL);
  struct tm gmt;
  gmtime_r(&now, &gmt);

  char date[100];
  strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", &gmt);
  
	if (file_fd == -1) { 

    char response[512];

    // shows us why open () failed, ENOENT means no such file exists
    // open() changes the value of errno so you can check what specific failure it is afterward
		if (errno == ENOENT) { 

      // snprinf allows you to combine fixed text with values calculated after program runs
      snprintf(response, sizeof(response),
          "HTTP/1.1 404 Not Found\r\n"
          "Content-Type: text/plain\r\n"
          "Content-Length: 13\r\n"
          "Date: %s\r\n"
          "Connection: close\r\n"
          "\r\n"
          "404 Not Found",
          date);

      send(connfd, response, strlen(response), 0);
      return;
		} 	
    // EACCES means no authorized access to that file
		else if (errno == EACCES){

      snprintf(response, sizeof(response),
          "HTTP/1.1 403 Forbidden\r\n"
          "Content-Type: text/plain\r\n"
          "Content-Length: 13\r\n"
          "Date: %s\r\n"
          "Connection: close\r\n"
          "\r\n"
          "403 Forbidden",
          date);

      send(connfd, response, strlen(response), 0);
      return;
		} 	
		else {
			perror("open"); //print another error, if one occured
      return;
		}
	}
  
  const char *extension = strrchr(path, '.');
  const char *content_type = "";

  if (extension != NULL) {
      if (strcmp(extension, ".gif") == 0)
          content_type = "image/gif";
      else if (strcmp(extension, ".txt") == 0)
          content_type = "text/plain";
      else if (strcmp(extension, ".jpg") == 0)
          content_type = "image/jpeg";
      else if (strcmp(extension, ".html") == 0)
          content_type = "text/html";
  }


  struct stat file_info;

  // check if file allows you to get meta data info
  if (fstat(file_fd, &file_info) == -1) {
      perror("fstat");
      close(file_fd);
      return;
  }

  char headers[512];

  snprintf(headers, sizeof(headers),
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: %s\r\n"
      "Content-Length: %ld\r\n"
      "Date: %s\r\n"
      "\r\n",
      content_type, (long)file_info.st_size, date);

  send(connfd, headers, strlen(headers), 0);


  // use stat library to check read permission 

	char buffer[1048];
	ssize_t bytes_read; 
	// Read a chunk from the file into the buffer
	while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
    // send buffer information to client
		send(connfd, buffer, bytes_read, 0); 
	// saves the number of bytes read in bytes read returns a positive number afterwards 
	// for write() sends the bytes to the client 
	} 
	if (bytes_read == -1){
		perror("read"); 
    exit(1);
	}
	close (file_fd);
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

  /* first, set necessary fields in myaddr struct to hold address and port */
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

  while (1) {
    /* recv: read input string from the client */
    bzero(buf, BUFSIZE);

    // if this is the second time this is visited, the server would pause here and wait for 
    // client to send something
    num_read = recv(connfd, buf, BUFSIZE, 0);
    if (num_read < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        printf("Connection timed out\n");
      }
      else {
        printf("ERROR reading from socket, or connection time out.\n");
      }
      break;
    }
    printf("server received %d bytes: %s\n", num_read, buf);

    /* parse string to ensure request format is good; if bad, close connection, if it's HTTP/1.0 format*/
    int parseStringResult = parseString(buf, &result);

    // set date and time information
    time_t now = time(NULL);
    struct tm gmt;
    gmtime_r(&now, &gmt);

    char date[100];
    strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", &gmt);
    char response[512];

    // make a failure response string that is sent to client
    if (parseStringResult != 0) {
      snprintf(response, sizeof(response),
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 15\r\n"
            "Date: %s\r\n"
            "Connection: close\r\n"
            "\r\n"
            "400 Bad Request",
            date);

      send(connfd, response, strlen(response), 0);
      close(connfd);
      return NULL;
    }

    /* find file contexts and check that file exists, can be read, and store file contents in some way */

    // adds "files" to path name 
    // needed because the browser requests stuff like /index.html which may exist on server side
    // but in a folder like "files" with the path name files/index.html. We decide to put 
    // index.html in files so that was the only reason these next two lines were added

    char file_path[PATH_MAX + 6];

    snprintf(file_path, sizeof(file_path), "files%s", result.path);


    // send response headers + response body containing the file info if found
    read_file(connfd, file_path);

    // to immediately close the connection break the while loop;
    // HTTP/1.0 requests close connection after succesful response

    if (strcmp(result.version, "HTTP/1.0") == 0) {
      break;
    };

    // hold a duration (does nothing on its own), this is for 5 seconds 0 miliseconds
    struct timeval tv = {5, 0};
    
    // for connfd make recv() stop waiting after 5 seconds if no data arrives
    // this doesnt wait five seconds itself it just configures how later recieve calls behave
    setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    

    }

  close(connfd);
  printf("Connection closed\n");
  return NULL;
}
 