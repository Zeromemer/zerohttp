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
	int url_invalid = parse_url(req.url, parsed_url);


	printf("%s:%d:\n\tmethod: %s\n\turl: %s (invalid: %d , parsed: %s)\n\tver: %s \n\tvalid: %d\n",
			ip,
			conn.cli.sin_port,
			req.method,
			req.url,
			url_invalid,
			parsed_url,
			req.ver,
			req_valid);
	printf("\theaders:\n");
	for (int i = 0; i < req.headers_len; i++) {
		printf("\t\t%s: %s\n", req.headers[i].name, req.headers[i].value);
	}

	res_t res = {0};	
	if (req_valid) {
		res.ver = "HTTP/1.1";
		res.status = 200;
		res.msg = stringify_status_code(res.status);

		header_t headers[] = {
			{"Server", "zerohttp"},
			{"Connection", "close"}
		};

		res.headers = headers;
		res.headers_len = STAT_ARR_SIZE(headers);
	} else {
		res.ver = "HTTP/1.1";
		res.status = 400;
		res.msg = stringify_status_code(res.status);

		header_t headers[] = {
			{"Server", "zerohttp"},
			{"Connection", "close"}
		};

		res.headers = headers;
		res.headers_len = STAT_ARR_SIZE(headers);
	}

	send_res(conn.fd, res);

	free_req(req);
	close(conn.fd);
	
	return NULL;
}
