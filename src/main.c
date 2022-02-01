#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <arpa/inet.h>

#include "include/tcp.h"
#include "include/http.h"
#include "include/misc.h"

int main(int argc, char** argv) {
	int sockfd = create_bound_socket(42069);

	socket_listen(sockfd, 5);

	conn_t conn = await_connection(sockfd);
	char *ip = inet_ntoa(conn.cli.sin_addr);
	
	dprintf(conn.fd, "sussy baka current unix time is %d\n", (int)time(NULL));

	request req = parse_req(conn.fd);

	printf("%s%d: method: %s url: %s ver: %s\n",
			ip,
			conn.cli.sin_port,
			req.method,
			req.uri,
			req.ver);

	// char* meth = dget_word(conn.fd);
	// printf("method = %s\n", meth);
	// free(meth);

	close(conn.fd);
	close(sockfd);
}
