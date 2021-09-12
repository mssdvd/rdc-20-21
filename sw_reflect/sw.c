#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>

int tmp;

struct header {
  char *n;
  char *v;
} h[100];

int main() {
  struct sockaddr_in addr, remote_addr;
  int i, s, t, s2, len;
  FILE *fin;
  int yes = 1;
  char request[5000], response[10000];
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) {
    perror("Socket fallita");
    return 1;
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8382);
  addr.sin_addr.s_addr = 0;
  t = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if (t == -1) {
    perror("setsockopt fallita");
    return 1;
  }
  if (bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
    perror("bind fallita");
    return 1;
  }
  if (listen(s, 5) == -1) {
    perror("Listen Fallita");
    return 1;
  }
  len = sizeof(struct sockaddr_in);
  while (1) {
    s2 = accept(s, (struct sockaddr *)&remote_addr, &len);
    if (s2 == -1) {
      perror("Accept Fallita");
      return 1;
    }
    t = read(s2, request, 4999);
    if (t == -1) {
      perror("Read fallita");
      return 1;
    }

    request[t] = 0;

    printf("%s", request);

    for (i = 0; request[i] != ' '; i++)
      ;

    int k = 0;
    char path[32];
    for (; request[k + i + 1] != ' '; ++k) {
      path[k] = request[k + i + 1];
    }

    // path[k] = 0;

    printf("path=%s\n", path);

    if (strcmp(path, "/reflect") == 0) {
      sprintf(response, "HTTP/1.1 200 OK\r\n\r\n");
      write(s2, response, strlen(response));

      // client request
      write(s2, request, t);
      write(s2, "\r\n", 2);

      // client ip
      struct sockaddr_in *addr_in = (struct sockaddr_in *)&remote_addr;
      char *remote_ip = inet_ntoa(addr_in->sin_addr);
      write(s2, remote_ip, strlen(remote_ip));
      write(s2, "\r\n", 2);

      // client port
      sprintf(response, "%d", addr_in->sin_port);
      write(s2, response, strlen(response));
    }
    close(s2);
  }
}

/*
   statusline = h[0].n=response;
   for( j=0,k=0; read(s,response+j,1);j++){
   if(response[j]==':' && (h[k].v==0) ){
   response[j]=0;
   h[k].v=response+j+1;
   }
   else if((response[j]=='\n') && (response[j-1]=='\r') ){
   response[j-1]=0;
   if(h[k].n[0]==0) break;
   h[++k].n=response+j+1;
   }
   }
   entity_length = -1;
   printf("Status line = %s\n",statusline);
   for(i=1;i<k;i++){
   if(strcmp(h[i].n,"Content-Length")==0){
   entity_length=atoi(h[i].v);
   printf("* (%d) ",entity_length);
   }
   printf("%s ----> %s\n",h[i].n, h[i].v);
   }
   entity_length=22000;
   if(entity_length == -1) entity_length=1000000;
   entity = (char * ) malloc(entity_length);
   for(j=0; (t=read(s,entity+j,entity_length-j))>0;j+=t);
//if ( t == -1) { perror("Read fallita"); return 1;}
printf("j= %d\n",j);

for(i=0;i<j;i++) printf("%c",entity[i]);
}
*/
