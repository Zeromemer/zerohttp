#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include "include/tcp.h"
#include "include/http.h"
#include "include/misc.h"
#include "include/xmalloc.h"

#define PORT 42069

void *serve_request(void *conn_p) {
	conn_t conn = *(conn_t*)conn_p;
	free(conn_p);
	req_t req = {0};
	int req_valid = parse_req(conn.fd, &req);

	char *ip = inet_ntoa(conn.cli.sin_addr);

	printf("%s:%d:\n\tmethod: %s\n\turl: %s\n\tver: %s \n\tvalid: %d\n",
			ip,
			conn.cli.sin_port,
			req.method,
			req.url,
			req.ver,
			req_valid);
	printf("\theaders:\n");
	for (int i = 0; i < req.headers_len; i++) {
		printf("\t\t%s: %s\n", req.headers[i].name, req.headers[i].value);
	}
	
	if (req_valid) {
		dprintf(conn.fd, "HTTP/1.1 204 No Content\r\n\r\n");
	} else {
		dprintf(conn.fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
	}

	free_req(req);
	close(conn.fd);
	
	return NULL;
}

int main(int argc, char **argv) {
	int sockfd = create_bound_socket(PORT);

	socket_listen(sockfd, 5);

	printf("Listening on port %d\n", PORT);
	for (;;) {
		conn_t *conn = xcalloc(1, sizeof(conn_t));
		int status = await_connection(sockfd, conn);
		
		if (status) {
			fprintf(stderr, "Could not accept client: %s\n", strerror(errno));
			continue;
		}

		pthread_t thread;
		pthread_create(&thread, NULL, serve_request, conn);
	}
	close(sockfd);
}
