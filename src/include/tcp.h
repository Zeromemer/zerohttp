#ifndef TCP_H
#define TCP_H

#include <netinet/in.h>

typedef struct {
	int fd;
	struct sockaddr_in data;
	time_t time_created;
} conn_t;

int create_bound_socket(int port);

void socket_listen(int sockfd, int backlog);

int await_connection(int sockfd, conn_t *conn);

#endif
