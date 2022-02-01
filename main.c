#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#define MAX 80
#define PORT 42069

#include "include/tcp.h"

int sockfd;

void cleanup() {
	puts("exiting...");
	close(sockfd);
}

int main(int argc, char** argv) {
	atexit(cleanup);
	signal(SIGINT, cleanup);
	struct sockaddr_in cli;

	sockfd = create_bound_socket(PORT);

	socket_listen(sockfd, 5);

	printf("Awaiting connection...\n");
	conn_t conn = await_connection(sockfd);

	char* ip = inet_ntoa(conn.cli.sin_addr);

	printf("Connection from %s:%u\n", ip, conn.cli.sin_port);
	// raise(SIGTRAP);

	dprintf(conn.fd, "HTTP/1.1 200 OK\r\n");
	dprintf(conn.fd, "Server: testing\r\n");
	dprintf(conn.fd, "Content-Type: text/html\r\n");
	dprintf(conn.fd, "Connection: Closed\r\n\r\n");

	dprintf(conn.fd, "<h1>Hello, World</h1>\r\n\r\n");

	// After chatting close the socket
	close(conn.fd);

	return 0;
}
