#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>
#define CHUNK_SIZE 20

int tmp;

struct header {
  char *n;
  char *v;
} h[100];

int main() {
  struct sockaddr_in addr, remote_addr;
  int i, j, k, s, t, s2, len, file_len;
  int c;
  FILE *fin;
  int yes = 1;
  char *method, *path, *ver;
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
    method = request;
    for (i = 0; request[i] != ' '; i++) {
    }
    request[i] = 0;
    path = request + i + 1;
    for (i++; request[i] != ' '; i++)
      ;
    request[i] = 0;
    ver = request + i + 1;
    for (i++; request[i] != '\r'; i++)
      ;
    request[i] = 0;
    printf("method=%s path=%s ver=%s\n", method, path, ver);
    if ((fin = fopen(path + 1, "rt")) == NULL) {
      sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
      write(s2, response, strlen(response));
    } else {
      sprintf(response,
              "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
      write(s2, response, strlen(response));

      /* File length */
      fseek(fin, 0, SEEK_END);
      file_len = ftell(fin);
      rewind(fin);

      int chunk_size;
      while (1) {
        chunk_size = file_len - ftell(fin) < CHUNK_SIZE ? file_len - ftell(fin)
                                                        : CHUNK_SIZE;
        sprintf(response, "%x\r\n", chunk_size);
        write(s2, response, strlen(response));

        for (i = 0; i < chunk_size; ++i) {
          c = fgetc(fin);
          write(s2, &c, 1);
        }

        write(s2, "\r\n", 2);

        if (!chunk_size)
          break;
      }

      fclose(fin);
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
