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

#define MAX_THREAD_COUNT 12

static int thread_count = 0;
static int maxed_out = 0;
pthread_mutex_t tc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER;

void signal_handler(int signal) {
	printf("catch signal %d\n", signal);
}

void *conn_handle(void *conn_p) {
	serve_request(conn_p);
	pthread_mutex_lock(&tc_lock);
	thread_count--;
	if (maxed_out) {
		maxed_out = 0;
		pthread_mutex_unlock(&main_lock);
	}
	pthread_mutex_unlock(&tc_lock);
	return NULL;
}

int main(int argc, char **argv) {
	char *port_s = argv[1];
	int port = port_s ? atoi(port_s) : 42069;
	if (port > 65535 || port < 1) {
		fprintf(stderr, "Port %d out of range\n", port);
		return 1;
	}
	signal(SIGPIPE, signal_handler);
	
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

		if (maxed_out) {
			pthread_mutex_lock(&main_lock);
			pthread_mutex_unlock(&main_lock);
		}
		
		pthread_mutex_lock(&tc_lock);
		thread_count++;
		if (thread_count >= MAX_THREAD_COUNT) {
			maxed_out = 1;
			pthread_mutex_lock(&main_lock);
		}
		pthread_mutex_unlock(&tc_lock);

		pthread_t thread;
		pthread_create(&thread, NULL, conn_handle, conn);
		pthread_detach(thread);
	}
}