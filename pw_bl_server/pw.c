#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>

struct hostent *he;
struct sockaddr_in local, remote, server;
char request[2000], response[2000], request2[2000], response2[2000];
char *method, *path, *version, *host, *scheme, *resource, *port;
struct headers {
  char *n;
  char *v;
} h[30];

int main() {
  FILE *f;
  char command[100];
  int i, s, t, s2, s3, n, len, c, yes = 1, j, k, pid;
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) {
    perror("Socket Fallita\n");
    return 1;
  }
  local.sin_family = AF_INET;
  local.sin_port = htons(7382);
  local.sin_addr.s_addr = 0;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  t = bind(s, (struct sockaddr *)&local, sizeof(struct sockaddr_in));
  if (t == -1) {
    perror("Bind Fallita \n");
    return 1;
  }
  t = listen(s, 10);
  if (t == -1) {
    perror("Listen Fallita \n");
    return 1;
  }
  while (1) {
    f = NULL;
    remote.sin_family = AF_INET;
    len = sizeof(struct sockaddr_in);
    s2 = accept(s, (struct sockaddr *)&remote, &len);
    if (fork())
      continue;
    if (s2 == -1) {
      perror("Accept Fallita\n");
      return 1;
    }

    j = 0;
    k = 0;
    h[k].n = request;
    while (read(s2, request + j, 1)) {
      if ((request[j] == '\n') && (request[j - 1] == '\r')) {
        request[j - 1] = 0;
        if (h[k].n[0] == 0)
          break;
        h[++k].n = request + j + 1;
      }
      if (request[j] == ':' && (h[k].v == 0) && k) {
        request[j] = 0;
        h[k].v = request + j + 1;
      }
      j++;
    }
    printf("%s", request);
    method = request;
    for (i = 0; (i < 2000) && (request[i] != ' '); i++)
      ;
    request[i] = 0;
    path = request + i + 1;
    for (; (i < 2000) && (request[i] != ' '); i++)
      ;
    request[i] = 0;
    version = request + i + 1;

    printf("Method = %s, path = %s , version = %s\n", method, path, version);
    if (!strcmp("GET", method)) {
      //  http://www.google.com/path
      scheme = path;
      for (i = 0; path[i] != ':'; i++)
        ;
      path[i] = 0;
      host = path + i + 3;
      for (i = i + 3; path[i] != '/'; i++)
        ;
      path[i] = 0;
      resource = path + i + 1;
      printf("Scheme=%s, host=%s, resource = %s\n", scheme, host, resource);
      he = gethostbyname(host);
      if (he == NULL) {
        printf("Gethostbyname Fallita\n");
        return 1;
      }
      printf("Server address = %u.%u.%u.%u\n", (unsigned char)he->h_addr[0],
             (unsigned char)he->h_addr[1], (unsigned char)he->h_addr[2],
             (unsigned char)he->h_addr[3]);
      s3 = socket(AF_INET, SOCK_STREAM, 0);
      if (s3 == -1) {
        perror("Socket to server fallita");
        return 1;
      }
      server.sin_family = AF_INET;
      server.sin_port = htons(80);
      server.sin_addr.s_addr = *(unsigned int *)he->h_addr;

      // TODO: Implementeare whitelist anche per GET
      //       Farlo per connect non è sufficiente

      t = connect(s3, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
      if (t == -1) {
        perror("Connect to server fallita");
        return 1;
      }
      sprintf(request2,
              "GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n\r\n",
              resource, host);
      write(s3, request2, strlen(request2));
      while (t = read(s3, response2, 2000))
        write(s2, response2, t);
      shutdown(s3, SHUT_RDWR);
      close(s3);
    } else if (!strcmp("CONNECT", method)) { // it is a connect  host:port
      host = path;
      for (i = 0; path[i] != ':'; i++)
        ;
      path[i] = 0;
      port = path + i + 1;
      printf("host:%s, port:%s\n", host, port);
      printf("Connect skipped ...\n");
      he = gethostbyname(host);
      if (he == NULL) {
        printf("Gethostbyname Fallita\n");
        return 1;
      }
      printf("Connecting to address = %u.%u.%u.%u\n",
             (unsigned char)he->h_addr[0], (unsigned char)he->h_addr[1],
             (unsigned char)he->h_addr[2], (unsigned char)he->h_addr[3]);
      s3 = socket(AF_INET, SOCK_STREAM, 0);

      if (s3 == -1) {
        perror("Socket to server fallita");
        return 1;
      }
      server.sin_family = AF_INET;
      server.sin_port = htons((unsigned short)atoi(port));
      server.sin_addr.s_addr = *(unsigned int *)he->h_addr;

      char whitelist[4][4] = {{88, 80, 187, 84},
                              {1, 1, 1, 1},
                              {172, 217, 23, 110},
                              {147, 162, 235, 155}};

      /*
      struct hostent *google = gethostbyname("google.com");
      memcpy(&whitelist[2], google->h_addr, sizeof(google->h_addr));
      struct hostent *unipd = gethostbyname("unipd.it");
      memcpy(&whitelist[3], unipd->h_addr, sizeof(unipd->h_addr));
      */

      int is_whitelisted = 0;

      for (int i = 0; i < 4; ++i) {
        if (whitelist[i][0] == he->h_addr[0] &&
            whitelist[i][1] == he->h_addr[1] &&
            whitelist[i][2] == he->h_addr[2] &&
            whitelist[i][3] == he->h_addr[3]) {
          is_whitelisted = 1;
        }
      }

      if (is_whitelisted == 0) {
        printf("\nHost: %s is blacklisted\n", host);
        shutdown(s2, SHUT_RDWR);
        close(s2);
        exit(0);
      }

      printf("\nHost: %s is whitelisted\n", host);

      t = connect(s3, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
      if (t == -1) {
        perror("Connect to server fallita");
        exit(0);
      }
      sprintf(response, "HTTP/1.1 200 Established\r\n\r\n");
      write(s2, response, strlen(response));
      // <==============
      if (!(pid = fork())) { // Child
        while (t = read(s2, request2, 2000)) {
          write(s3, request2, t);
          // printf("CL >>>(%d)%s \n",t,host); //SOLO PER CHECK
        }
        exit(0);
      } else { // Parent
        while (t = read(s3, response2, 2000)) {
          write(s2, response2, t);
          // printf("CL <<<(%d)%s \n",t,host);
        }
        kill(pid, SIGTERM);
        shutdown(s3, SHUT_RDWR);
        close(s3);
      }
    }
    shutdown(s2, SHUT_RDWR);
    close(s2);
    exit(0);
  }
}
