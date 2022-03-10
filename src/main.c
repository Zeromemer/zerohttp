#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "include/tcp.h"
#include "include/http.h"
#include "include/misc.h"
#include "include/xmalloc.h"
#include "include/req_handl.h"

int main(int argc, char **argv) {
	char *port_s = argv[1];
	int port = port_s ? atoi(port_s) : 42069;
	if (port > 65535) {
		fprintf(stderr, "Could not bind socket: Port number out of range\n"); // if libc isn't going to check the port number, I will
		return 1;
	}

	struct sigaction sig_conf = {{SIG_IGN}};
	sigaction(SIGPIPE, &sig_conf, NULL);
	
	int sockfd = create_bound_socket(port);

	socket_listen(sockfd, 5);

	printf("Listening on port %d\n", port);
	for (;;) {
		conn_t *conn = xcalloc(1, sizeof(conn_t));
		int status = await_connection(sockfd, conn);
		
		if (status) {
			fprintf(stderr, "\033[31mERROR: \033[0mCould not accept client: %s\n", strerror(errno));
			continue;
		}

		pthread_t thread;
		pthread_attr_t attrs;
		pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread, NULL, serve_request, conn);
	}
}
