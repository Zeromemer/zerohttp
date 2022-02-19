#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "include/http.h"

void *serve_request(void *conn_p) {
	conn_t conn = *(conn_t*)conn_p;
	free(conn_p);
	req_t req = {0};
	int req_valid = parse_req(conn.fd, &req);

	char *ip = inet_ntoa(conn.cli.sin_addr);

	printf("%s:%d:\n\tmethod: %s\n\turl: %s\n\tver: %s \n\tvalid: %d\n",
			ip,
			conn.cli.sin_port,
			req.method,
			req.url,
			req.ver,
			req_valid);
	printf("\theaders:\n");
	for (int i = 0; i < req.headers_len; i++) {
		printf("\t\t%s: %s\n", req.headers[i].name, req.headers[i].value);
	}
	
	if (req_valid) {
		dprintf(conn.fd, "HTTP/1.1 204 No Content\r\n\r\n");
	} else {
		dprintf(conn.fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
	}

	free_req(req);
	close(conn.fd);
	
	return NULL;
}
