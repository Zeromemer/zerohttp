#ifndef TCP_H
#define TCP_H

#include <netinet/in.h>

typedef struct {
	int fd;
	struct sockaddr_in cli;
	socklen_t len;
} conn_t;

int create_bound_socket(int port);

void socket_listen(int sockfd, int backlog);

conn_t await_connection(int sockfd);

#endif
