#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "include/req_handl.h"
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

	
	res_t res = {0};
	if (req_valid) {
		res.ver = "HTTP/1.1";
		res.status = 204;
		res.msg = "No Content";
	} else {
		res.ver = "HTTP/1.1";
		res.status = 400;
		res.msg = "Invalid Request";
	}

	send_res(conn.fd, res);
	end_res(conn.fd);

	free_req(req);
	close(conn.fd);
	
	return NULL;
}
