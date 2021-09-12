/* Scrittura in buffer con read(2) senza copie, dim buffer suff grande
 * da contenere homepage di google. Alla fine una sola printf(3) del buffer.
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFF_SIZE 60000

int main() {
  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  addr.sin_addr.s_addr = inet_addr("216.58.213.100"); /* google.com */

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("Socket fail");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) ==
      -1) {
    perror("Connect fail");
    return -1;
  }

  char request[100], response[BUFF_SIZE];

  sprintf(request, "GET / HTTP/1.0\r\n\r\n");

  if (write(sock, request, strlen(request)) == -1) {
    perror("Write fail");
    return -1;
  }

  for (int i, j = 0; (j = read(sock, response + i, BUFF_SIZE - i)) > 0; i += j)
    ;

  printf("%s\n", response);

  return 0;
}
