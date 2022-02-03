#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "include/tcp.h"
#include "include/http.h"
#include "include/misc.h"

#define PORT 42069

void *serve_request(void *conn_v) {
	conn_t conn = *(conn_t*)conn_v;


	char *ip = inet_ntoa(conn.cli.sin_addr);

	req_t req = {0};
	int valid = parse_req(conn.fd, &req);

	printf("%s:%d:\n\tmethod: %s\n\turl: %s\n\tver: %s \n\tvalid: %d\n",
			ip,
			conn.cli.sin_port,
			req.method,
			req.url,
			req.ver,
			valid);
	
	// dprintf(conn.fd, "sussy baka current unix time is %d\n", (int)time(NULL));
	
	free_req(req);
	close(conn.fd);
	
	return NULL;
}

int main(int argc, char **argv) {
	int sockfd = create_bound_socket(PORT);

	socket_listen(sockfd, 5);

	printf("Listening on port %d\n", PORT);
	for (;;) {
		conn_t conn = await_connection(sockfd);
		
		pthread_t thread;
		pthread_create(&thread, NULL, serve_request, (void*)&conn);
		
	}
	close(sockfd);
}
