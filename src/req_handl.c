#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "include/req_handl.h"
#include "include/http.h"
#include "include/xmalloc.h"
#include "include/misc.h"

char *srcs_dir = "./req_src";

void serve_regular_request(conn_t conn, req_t req, char *parsed_url) {
	if (!strcmp(parsed_url, "/debug")) {
		send_res_status(conn.fd, "HTTP/1.1", 200, "OK");

		send_res_header(conn.fd, "Server", "zerohttp");
		send_res_header(conn.fd, "Connection", "close");
		send_res_header(conn.fd, NULL, NULL);

		dprintf(conn.fd, "%s:%d:\n\tmethod: %s\n\turl: %s (parsed: %s)\n\tver: %s \n\t",
				inet_ntoa(conn.cli.sin_addr),
				conn.cli.sin_port,
				req.method,
				req.url,
				parsed_url,
				req.ver);
		dprintf(conn.fd, "headers:\n");
		for (int i = 0; i < req.headers_len; i++) {
			dprintf(conn.fd, "\t\t%s: %s\n", req.headers[i].name, req.headers[i].value);
		}
		
		return;
	}

	char path[strlen(srcs_dir) + strlen(parsed_url)];

	strcat_mod(path, srcs_dir, parsed_url);
	printf("path: %s\n", path); 

	struct stat path_stat;
	if (lstat(path, &path_stat) != -1 && S_ISREG(path_stat.st_mode)) {
		int fd = open(path, O_RDONLY);

		send_res_status(conn.fd, "HTTP/1.1", 200, "OK");

		send_res_header(conn.fd, "Server", "zerohttp");
		send_res_header(conn.fd, "Connection", "close");
		send_res_header(conn.fd, NULL, NULL);

		sendfile(conn.fd, fd, NULL, path_stat.st_size);
		
		return;
	}

	send_res_status(conn.fd, "HTTP/1.1", 404, "Not Found");
	
	send_res_header(conn.fd, "Server", "zerohttp");
	send_res_header(conn.fd, "Connection", "close");
	send_res_header(conn.fd, NULL, NULL);
}

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

	if (req_valid && url_valid && check_url(parsed_url)) {
		serve_regular_request(conn, req, parsed_url);
	} else {
		int status = 400;
		send_res_status(conn.fd, "HTTP/1.1", status, "Bad Request");
		
		send_res_header(conn.fd, "Server", "zerohttp");
		send_res_header(conn.fd, "Connection", "close");
		send_res_header(conn.fd, NULL, NULL);
	}

	free_req(req);
	close(conn.fd);
	
	return NULL;
}
