/* Implementazione cache HTTP/1.0 Last-Modified */
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define BUFF_SIZE 60000
#define HEADER_SIZE 100

typedef struct header {
	char *n;
	char *v;
} header;

int main() {
	struct sockaddr_in addr;
	char request[100], response[BUFF_SIZE], file_name[50];
	int sock, nentries, content_len;
	header h[100];
	char *statusline;
	char *content;
	char *last_modified;

	FILE *fo;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	addr.sin_addr.s_addr = inet_addr("93.184.216.34"); /* www.example.com */

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		perror("Socket fail");
		return -1;
	}

	if (connect(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1) {
		perror("Connect fail");
		return -1;
	}

	char host[] = "www.example.com";
	char resource[] = "/pippo/wow/";
	
	sprintf(request, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", resource, host);

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
		} else if (strcmp(h[i].n, "Last-Modified") == 0) {
			last_modified = h[i].v;
		}
		printf("%s --> %s\n", h[i].n, h[i].v);
	}

	if (content_len == -1) content_len = BUFF_SIZE;

	content = (char *) malloc(content_len + 1);

	/* Lettura message body */
	int i = 0;
	for(int j = 0; (j = read(sock, content + i, content_len - i)) > 0; i += j);
	content[i] = 0;

 	// printf("%s\n", content);

	for (int i = 0; resource[i]; ++i) {
		if (resource[i] == '/')
			resource[i] = '_';
	}

	sprintf(file_name, "./cache/%s%s", host, resource);

	printf("%s\n", file_name);

	fo = fopen(file_name, "r");
	time_t cached_epoch = 0;
	
	if (fo) {
		char *cached_time = malloc(100); size_t len = 100;
		getline(&cached_time, &len, fo);
		fclose(fo);
		cached_epoch = atol(cached_time);
	}
	struct tm tm;
	strptime(last_modified, "%a, %d %b %Y %H:%M:%S %Z", &tm);
	time_t last_mod_epoch = mktime(&tm);

	if (last_mod_epoch > cached_epoch) {
		printf("New updates for %s\n",file_name);
		fo = fopen(file_name, "w+"); // overwrite file
		fprintf(fo, "%ld\n%s", last_mod_epoch, content);
		fclose(fo);
	}	

	return 0;
}
