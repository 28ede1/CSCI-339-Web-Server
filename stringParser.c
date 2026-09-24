#include <stdio.h>
#include <string.h>
#include <limits.h>

typedef struct {
  char method[4]; // method length, only GET will be supported, so use 4 (GET + "\0")
  char path[PATH_MAX]; // length of longest path of whatever machine the code compiles on
  char version[9]; // version length, HTTP/1.1 or HTTP/1.0 (version + "\0")
} request_params;


int parseString(char *str, request_params *result);

int main(int argc, char **argv) {
  char request[] = "GET /index.html HTTP/1.1";

  request_params result;
  result.method[0] = '\0';
  result.path[0] = '\0';
  result.version[0] = '\0';

  int parseStringResult = parseString(request, &result);
  if (parseStringResult) {
    printf("Invalid request\n");
    return 1;
}

  printf("Method: %s\n", result.method);
  printf("Path: %s\n", result.path);
  printf("Version: %s\n", result.version);

  return 0;
}

int parseString(char *str, request_params *result) {

  char delimiter[] = " ";

  char *portion1 = strtok(str, delimiter);

  char *portion2 = strtok(NULL, delimiter);

  char *portion3 = strtok(NULL, delimiter);

  char *portion4 = strtok(NULL, delimiter);

  if ((portion1 == NULL) || (portion2 == NULL) || (portion3 == NULL)) {
    printf("Invalid request\n");
    return 1;
  }

  if ((portion4 != NULL)) {
    printf("Invalid request\n");
    return 1;
  }

  if ((strcmp(portion1, "GET") != 0) || ((strcmp(portion3, "HTTP/1.1") != 0) && (strcmp(portion3, "HTTP/1.0") != 0) )){
    printf("Invalid request\n");
    return 1;
  }
  
  strcpy(result->method, portion1);
  strcpy(result->path, portion2);
  strcpy(result->version, portion3);
  return 0;
}

// strtok, strncmp, strcmp
// 
// 