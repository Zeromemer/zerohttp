#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "include/req_handl.h"
#include "include/http.h"
#include "include/xmalloc.h"
#include "include/misc.h"

void *serve_request(void *conn_p) {
	conn_t conn = *(conn_t*)conn_p;
	xfree(conn_p);
	req_t req = {0};
	int req_valid = parse_req(conn.fd, &req);

	char *ip = inet_ntoa(conn.cli.sin_addr);

	char parsed_url[strlen(req.url) + 1];
	int url_valid = parse_url(req.url, strlen(req.url), parsed_url);


	printf("%s:%d:\n\tmethod: %s\n\turl: %s (valid: %d, parsed: %s)\n\tver: %s \n\tvalid: %d\n",
			ip,
			conn.cli.sin_port,
			req.method,
			req.url,
			url_valid,
			parsed_url,
			req.ver,
			req_valid);
	printf("\theaders:\n");
	for (int i = 0; i < req.headers_len; i++) {
		printf("\t\t%s: %s\n", req.headers[i].name, req.headers[i].value);
	}

	if (req_valid && url_valid) {
		int status = 200;
		send_res_status(conn.fd, "HTTP/1.1", status, stringify_status_code(status));
		
		send_res_header(conn.fd, "Server", "zerohttp");
		send_res_header(conn.fd, "Connection", "close");
		send_res_headers_end(conn.fd);
		
		dprintf(conn.fd, "<h1>Hello, World!<h1>");
	} else {
		int status = 400;
		send_res_status(conn.fd, "HTTP/1.1", status, stringify_status_code(status));
		
		send_res_header(conn.fd, "Server", "zerohttp");
		send_res_header(conn.fd, "Connection", "close");
		send_res_headers_end(conn.fd);
	}

	free_req(req);
	close(conn.fd);
	
	return NULL;
}
