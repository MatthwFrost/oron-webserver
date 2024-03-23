#include "common.h"


void read_file(const char* file_path){};

void send_res_headers(const int connfd, const int status, const char* content_type, long content_length){
  char headers[1024];
  sprintf(headers, "HTTP/1.0 %d OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", status, content_type, content_length);
  send(connfd, headers, strlen(headers), 0);
}

void send_file(const int connfd, const char* file_path){
  FILE *file = fopen(file_path, "rb");

  if(file == NULL){
    send_res_headers(connfd, 404, "text/html", 0);
    return;
  };

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  send_res_headers(connfd, 200, "text/html", file_size);
  

  char *buffer = (char *)malloc(file_size);
  fread(buffer, 1, file_size, file);
  fclose(file);

  send(connfd, buffer, file_size, 0);
  free(buffer);
}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    if(result == NULL)
      return "NULL";

    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void send_image(const int connfd, const char* file_path){
  printf("\nGetting image\n\n");
  
  char* file_loc = concat("static/", file_path);
  printf("PATH: %s\n", file_loc);
  FILE *file = fopen(file_loc, "rb");

  if(file == NULL){
    send_res_headers(connfd, 404, "image/jpeg", 0);
    return;
  };

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  send_res_headers(connfd, 200, "image/jpg", file_size);

  char *buffer = (char *)malloc(file_size);
  fread(buffer, 1, file_size, file);
  fclose(file);
  send(connfd, buffer, file_size, 0);
  free(buffer);
}

void send_404(const int connfd){
  FILE *file = fopen("404.html", "rb");

  if(file == NULL){
    send_res_headers(connfd, 404, "text/html", 0);
    return;
  };

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  send_res_headers(connfd, 404, "text/html", file_size);
  

  char *buffer = (char *)malloc(file_size);
  fread(buffer, 1, file_size, file);
  fclose(file);

  send(connfd, buffer, file_size, 0);
  free(buffer);
}

int request_type(const char* request){
  // Check for extension.
  // Return request type.
  // Page or assett request.
  char *extension = strrchr(request, '.');
  printf("\ntype: %s", extension);
  if (extension != NULL)
      return 1;   
  return 0;
}

void handle_request(const int connfd, const char* method, const char* path){
  printf("Request: %s\n%s", method, path);

  // Check for request type
  // Assett or Page.
  // 1: asset
  // 0: page
  int type = request_type(path);

  if(type == 0){
    if(strcmp(path, "/") == 0)
      send_file(connfd, "routes/index.html");
    else if(strcmp(path, "/page2") == 0)
      send_file(connfd, "routes/page2/page2.html");
    else send_404(connfd);
  }
  else {
    if (path[0] == '/') {
      memmove((void*)path, path + 1, strlen(path));
    }
    send_image(connfd, path);
  }
}

int main(){
  int connfd, n, yes = 1;
  uint8_t recvline[MAXLINE + 1];
  uint8_t buff[MAXLINE + 1];

  int s = socket(AF_INET, SOCK_STREAM, 0);
  if(s < 0)
    return 0;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
      fprintf(stderr, "ERROR on SOCKET: %s\n", strerror(errno));
      exit(1);
  }

  struct sockaddr_in addr = {
    AF_INET,
    htons(PORT_NUMBER),
    htonl(INADDR_ANY)
  };
  if(bind(s, (SA *) &addr, sizeof(addr))){
    fprintf(stderr, "ERROR on BIND: %s\n", strerror(errno));
    return 0;
  };

  if(listen(s, 10)){
    fprintf(stderr, "ERROR on LISTEN: %s\n", strerror(errno));
    return 0;  
  };

  printf("\n\nLaunching server on 'localhost:8080'");
  while(1){
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    char method[10];
    char path[255];
    char version[10];

    printf("\n\n\nWaiting for a connection\n\n");
    fflush(stdout);
    connfd = accept(s, (SA *)NULL, NULL);
    memset(recvline, 0, MAXLINE);
    while( (n = read(connfd, recvline, MAXLINE-1)) > 0){
      sscanf((char*) recvline, "%s %s %s", method, path, version);

      if(recvline[n-1] == '\n'){
        break;
      }
      memset(recvline, 0, MAXLINE);
    };
    if(n < 0)
      return 1;

    handle_request(connfd, method, path);
    close(connfd);
  }

  close(s);
  return 0;
}
