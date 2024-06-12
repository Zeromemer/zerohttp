#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <argp.h>

#include "include/main.h"
#include "include/tcp.h"
#include "include/xmalloc.h"
#include "include/req_handl.h"

static int thread_count = 0;
static int maxed_out = 0;
pthread_mutex_t tc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER;

static char doc[] = "A very basic http server made in C";
static char args_doc[] = "PORT";

static struct argp_option options[] = {
	{"threads", 'j', "NUM", 0, "Number of threads to use"},
	{"directory", 'd', "PATH", 0, "Directory from which to serve files"},
	{0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments_t *arguments = state->input;

	switch (key) {
		case 'p':
			arguments->port = atoi(arg);
			break;
		case 'd':
			arguments->directory = arg;
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num != 0)
				return ARGP_ERR_UNKNOWN;
			arguments->port = atoi(arg);
			break;
		default:
		return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

void signal_handler(int signal) {
	printf("catch signal %d\n", signal);
}

void *conn_handle(void *request_params) {
	serve_request(request_params);

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
	arguments_t arguments;
	arguments.port = 42069;
	arguments.threads = 6;
	arguments.directory = "content";
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	signal(SIGPIPE, signal_handler);
	
	int sockfd = create_bound_socket(arguments.port);
	socket_listen(sockfd, 5);
	printf("Serving files from \"%s\" on %d\n", arguments.directory, arguments.port);
	for (;;) {
		request_params_t *request_params = xcalloc(1, sizeof(request_params_t));
		request_params->arguments = arguments;
		int status = await_connection(sockfd, &request_params->conn);
		
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
		pthread_create(&thread, NULL, conn_handle, request_params);
		pthread_detach(thread);
	}
}
