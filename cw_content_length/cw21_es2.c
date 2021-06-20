/* Scrittura in buffer con read(2) senza copie, dim buffer presa da
 * Content-Length. Alla fine una sola printf(3) del buffer.
 */
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFF_SIZE 60000
#define HEADER_SIZE 100

typedef struct header {
	char *n;
	char *v;
} header;

int main() {
	struct sockaddr_in addr;
	char request[100], response[BUFF_SIZE];
	int sock, nentries, content_len;
	header h[100];
	char *statusline;
	char *content;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	addr.sin_addr.s_addr = inet_addr("216.58.213.100"); /* google.com */

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		perror("Socket fail");
		return -1;
	}

	if (connect(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1) {
		perror("Connect fail");
		return -1;
	}

	sprintf(request, "GET / HTTP/1.0\r\n\r\n");

	if (write(sock, request, strlen(request)) == -1) {
		perror("Write fail");
		return -1;
	}

	// memset(h, 0, sizeof(struct header) * 100);

	statusline = h[0].n = response;

	nentries = 0;
	for (int i = 0; read(sock, response + i, 1); ++i) {
		if (response[i] == ':' && (h[nentries].v == NULL)) {
			response[i] = 0; // string terminator
			h[nentries].v = response + i + 1;
		} else if ((response[i] == ' ') && (h[nentries].v == response + i)) {
			++h[nentries].v;
		} else if ((response[i] == '\n') && (response[i - 1] == '\r')) {
			response[i - 1] = 0;
			if (h[nentries].n[0] == 0) break;
			h[++nentries].n = response + i + 1;
		}
	}
	
	printf("Status line: %s\n", statusline);
	content_len = -1;
	for (int i = 0; i < nentries; ++i) {
		if (strcmp(h[i].n, "Content-Length") == 0) {
			content_len = atoi(h[i].v);
		}
		printf("%s --> %s\n", h[i].n, h[i].v);
	}

	if (content_len == -1) content_len = BUFF_SIZE;

	content = (char *) malloc(content_len + 1);

	/* Lettura message body */
	int i = 0;
	for(int j = 0; (j = read(sock, content + i, content_len - i)) > 0; i += j);
	content[i] = 0;

 	printf("%s\n", content);

	return 0;
}
