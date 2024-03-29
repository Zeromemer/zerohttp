#include "include/tcp.h"
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

/* Sourced from: https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/ */

int create_bound_socket(int port) {
	int sockfd;
	struct sockaddr_in servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		fprintf(stderr, "Could not open socket: %s\n", strerror(errno));
		exit(1);
	}

	int optval = 1;
	int opt = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (opt == -1) {
		fprintf(stderr, "Could not set option for socket: %s\n", strerror(errno));
	}

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
		fprintf(stderr, "Could not bind socket: %s\n", strerror(errno));
		exit(1);
	}

	return sockfd;
}

void socket_listen(int sockfd, int backlog) {
	if (listen(sockfd, backlog) == -1) {
		fprintf(stderr, "Could not listen to socket: %s\n", strerror(errno));
		exit(1);
	}
}

int await_connection(int sockfd, conn_t *conn) {
	socklen_t len = sizeof(conn->data);
	
	conn->fd = accept(sockfd, (struct sockaddr*)&conn->data, &len);
	conn->time_created = time(NULL);
	if (conn->fd < 0) {
		return 1;
	}
	
	return 0;
}
