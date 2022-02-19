#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "include/tcp.h"
#include "include/http.h"
#include "include/misc.h"
#include "include/xmalloc.h"
#include "include/req_handl.h"

#define PORT 42069

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
