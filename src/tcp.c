#include "include/tcp.h"
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

/* Original not split into function code: https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/ */

int create_bound_socket(int port) {
	int sockfd;
	struct sockaddr_in servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		fprintf(stderr, "Could not open socket %s\n", strerror(errno));
		exit(1);
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
	if (listen(sockfd, backlog)) {
		fprintf(stderr, "Could not listen to socket: %s\n", strerror(errno));
		exit(1);
	}
}

conn_t await_connection(int sockfd) {
	conn_t result = {0};
	result.len = sizeof(result.cli);
	
	result.fd = accept(sockfd, (struct sockaddr*)&result.cli, &result.len);
	if (result.fd < 0) {
		fprintf(stderr, "Could not accept client: %s\n", strerror(errno));
		exit(1);
	}
	
	return result;
}
