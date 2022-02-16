#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "include/tcp.h"
#include "include/http.h"
#include "include/misc.h"

#define PORT 42069

void *serve_request(void *client_p) {
	client_t client = *(client_t*)client_p;
	conn_t conn = client.conn;
	req_t req = client.req;

	char *ip = inet_ntoa(client.conn.cli.sin_addr);

	printf("%s:%d:\n\tmethod: %s\n\turl: %s\n\tver: %s \n\tvalid: %d\n",
			ip,
			conn.cli.sin_port,
			client.req.method,
			req.url,
			req.ver,
			client.req_valid);
	
	dprintf(conn.fd, "HTTP/1.1 204 No Content\r\n\r\n");
	
	free_req(req);
	close(conn.fd);
	
	return NULL;
}

int main(int argc, char **argv) {
	int sockfd = create_bound_socket(PORT);

	socket_listen(sockfd, 5);

	printf("Listening on port %d\n", PORT);
	client_t client = {0};
	for (;;) {
		client.conn = await_connection(sockfd);
		client.req_valid = parse_req(client.conn.fd, &client.req);
		
		pthread_t thread;
		pthread_create(&thread, NULL, serve_request, &client);	
	}
	close(sockfd);
}
