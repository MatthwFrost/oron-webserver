// Can I run a webpack file?
#include "include/common.h"

typedef struct Node {
    char* route;
    struct Node *next;
} Node;


Node *head = NULL;

Node *add_node(char* route){
  Node *new = NULL;

  if(head == NULL){
    new = malloc(sizeof(Node));
    if (new == NULL)
      return NULL;

    new->route = route;
    head = new;
    new->next = NULL;
  }else { 
    new = malloc(sizeof(Node));
    if (new == NULL)
      return NULL;

    new->route = route;
    new->next = head;
    head = new;
  }
  return new;
}

char* concat(const char *s1, const char *s2){
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    if(result == NULL)
      return "NULL";

    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int read_file_struct(){
  // Get all the files in routes.
  DIR *d;
  struct dirent *dir;
  d = opendir("routes");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0 && dir->d_type == DT_DIR){
         add_node(dir->d_name);
      }
    }
    closedir(d);
  }
  return(0);
}

void send_res_headers(const int connfd, const int status, const char* content_type, long content_length){
  char headers[1024];
  sprintf(headers, "HTTP/1.0 %d OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", status, content_type, content_length);
  send(connfd, headers, strlen(headers), 0);
}

void send_data(const int connfd, const char* file_path, const char* mime_type, int status){
  FILE *file = fopen(file_path, "rb");

  if(file == NULL){
    send_res_headers(connfd, status, mime_type, 0);
    return;
  };

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  send_res_headers(connfd, status, mime_type, file_size);

  char* buffer = (char *)malloc(file_size);
  fread(buffer, 1, file_size, file);
  fclose(file);

  send(connfd, buffer, file_size, 0);
  free(buffer);
}

char* request_type(const char* request){
  // Check for extension.
  // Return request type.
  // Page or assett request.
  char *extension = strrchr(request, '.');
  printf("\ntype: %s\n", extension);
  if (extension != NULL)
      return extension;   
  return NULL;
}

const char* str_trim(char* s, int remove){
  if (s[0] == remove){
    memmove((void*)s, s+ 1, strlen(s));
  }
  return s;
}

void handle_request(const int connfd, const char* method, char* path){
  printf("Request: %s\n%s", method, path);

  char* type = request_type(path);
  char* js_prefix = ".js";
  size_t js_prefix_len = strlen(js_prefix); 

  if(type == NULL){
    char* mime_type = "text/html";
    //char* fpath = str_trim(path);
    printf("Returned string: %s\n", path);
    if(strcmp(path, "/") == 0)
      send_data(connfd, "routes/index.html", mime_type, 200);
    else {
      // Remove '/' from start of request.
      const char* fpath = str_trim(path, '/');

      Node* current = head;
      char* file_con = NULL;
      bool FOUND = false;

      while(current != NULL){
        char* route = current->route;
        printf("\nroute: %s\n", route);
        if(strcmp(fpath, route) == 0){
          file_con = concat("routes/", route);
          file_con = concat(file_con, "/index.html");
          FOUND = true;
          break;
        }
        current = current->next;
      }
      if (FOUND){
        printf("\nFOUND: %s\n", file_con);
        send_data(connfd, file_con, mime_type, 200);
      } else {
        printf("Page not found\n");
        char* mime_type = "text/html";
        //send_404(connfd);
        send_data(connfd, "routes/404.html", mime_type, 404);
      }
    }
  }
  else if(strncmp(js_prefix, type, js_prefix_len) == 0){
    char* mime_type = "text/javascript";
    const char* fpath = str_trim(path, '/');
    char* route_path = concat("routes/", fpath); 
    send_data(connfd, route_path, mime_type, 200);
  }
  else {
    char* real_path = concat("static/", path);
    char* mime_type = concat("image/", type);
    send_data(connfd, real_path, mime_type, 200);
  }
}

int main(){
  
  read_file_struct();

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

  printf("\nLaunching server on 'localhost:8080'\n");
  while(1){
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    char method[10];
    char path[255];
    char version[10];

    printf("\nWaiting for a connection\n\n");
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
