#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <argp.h>

#include "include/tcp.h"
#include "include/xmalloc.h"
#include "include/req_handl.h"

static int thread_count = 0;
static int maxed_out = 0;
pthread_mutex_t tc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER;

struct arguments {
	int port;
	int threads;
};

static char doc[] = "A very basic http server made in C";

static struct argp_option options[] = {
	{"number", 'j', "NUM", 0, "Number of threads to use"},
	{"number", 'p', "NUM", 0, "Port to bind server to"},
	{0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;

	switch (key) {
		case 'p':
			arguments->port = atoi(arg);
			break;
		case 'j':
			arguments->threads = atoi(arg);
			break;
		default:
		return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, NULL, doc};

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
	struct arguments arguments;
	arguments.port = 42069;
	arguments.threads = 6;
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	signal(SIGPIPE, signal_handler);
	
	int sockfd = create_bound_socket(arguments.port);
	socket_listen(sockfd, 5);
	printf("Listening on port %d\n", arguments.port);
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
		if (thread_count >= arguments.threads) {
			maxed_out = 1;
			pthread_mutex_lock(&main_lock);
		}
		pthread_mutex_unlock(&tc_lock);

		pthread_t thread;
		pthread_create(&thread, NULL, conn_handle, conn);
		pthread_detach(thread);
	}
}
