#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE 60000
#define HEADER_SIZE 100

typedef struct header {
	char *n;
	char *v;
} header;

int main() {
	struct sockaddr_in addr;
	char request[100], response[BUFF_SIZE];
	int sock, nentries, content_index;
	int nbytes, chunk_size = 0;
	header h[100];
	char *content, chunk_size_str[32];

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

	sprintf(request, "GET / HTTP/1.1\r\nHost:www.google.com\r\n\r\n");
	//sprintf(request, "GET / HTTP/1.0\r\n\r\n");

	if (write(sock, request, strlen(request)) == -1) {
		perror("Write fail");
		return -1;
	}

	// memset(h, 0, sizeof(struct header) * 100);

	h[0].n = response;

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

	nbytes = -1;
	for (int i = 0; i < nentries; ++i) {
		printf("%s --> %s\n", h[i].n, h[i].v);
		if (strcmp(h[i].n, "Content-Length") == 0) {
			nbytes = atoi(h[i].v); }
		if (strcmp(h[i].n, "Transfer-Encoding") == 0 &&
			strcmp(h[i].v, "chunked") == 0) {
			printf("CHUNK\n");
			chunk_size = -1;
		}
	}

	content_index = 0;

	if (chunk_size == -1) {
		// Read chunk line
		content = (char *) malloc(1048576); // 1 MiB
		while (1) {
			int chunk_ext_len = -1;
			for (int i = 0; read(sock, chunk_size_str + i, 1) > 0; ++i) {
				if ((chunk_size_str[i] == '\n') && (chunk_size_str[i - 1] == '\r')) {
					chunk_size_str[i - 1 - chunk_ext_len] = 0; // NULL terminator
					break;
				}
				if (response[i] == ';' || chunk_ext_len >= 0) ++chunk_ext_len;
			}

			chunk_size = strtoul(chunk_size_str, NULL, 16);

			if (chunk_size == 0) break; // Maybe it doesn't read the trailer

			int i = 0;
			for(int j = 0; (j = read(sock, content + content_index + i, chunk_size - i)) > 0; i += j); 
			content_index += i;

			read(sock, response, 2); // discard CRLF
		}
	} else {
		if (nbytes == -1) nbytes = BUFF_SIZE;

		content = (char *) malloc(nbytes + 1);

		/* Lettura message body */
		for(int j = 0; (j = read(sock, content + content_index, nbytes - content_index)) > 0; content_index += j);
	}

	content[content_index] = 0;
	printf("%s\n", content);

	return 0;
}
