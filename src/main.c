#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <arpa/inet.h>

#include "include/tcp.h"
#include "include/http.h"
#include "include/misc.h"

int main(int argc, char **argv) {
	int sockfd = create_bound_socket(42069);

	socket_listen(sockfd, 5);

	conn_t conn = await_connection(sockfd);
	char *ip = inet_ntoa(conn.cli.sin_addr);
	
	dprintf(conn.fd, "sussy baka current unix time is %d\n", (int)time(NULL));

	request req = {0};
	int valid = parse_req(conn.fd, &req);

	printf("%s%d:\n\tmethod: %s\n\turl: %s\n\tver: %s \n\tvalid: %d\n",
			ip,
			conn.cli.sin_port,
			req.method,
			req.url,
			req.ver,
			valid);

	// char* meth = dget_word(conn.fd);
	// printf("method = %s\n", meth);
	// free(meth);

	close(conn.fd);
	close(sockfd);
}
