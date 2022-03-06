#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "include/req_handl.h"
#include "include/http.h"
#include "include/xmalloc.h"
#include "include/misc.h"
#include "include/mime.h"

#define CHUNK_SIZE 4096

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
		char *mime_type = path_to_mime(path);
		printf("mime: %s\n", mime_type);

		char content_length_s[sizeof("-2147483648")];
		sprintf(content_length_s, "%ld", path_stat.st_size);

		send_res_status(conn.fd, "HTTP/1.1", 200, "OK");

		send_res_header(conn.fd, "Server", "zerohttp");
		send_res_header(conn.fd, "Content-Type", mime_type);
		send_res_header(conn.fd, "Content-Length", content_length_s);
		send_res_header(conn.fd, "Connection", "keep-alive");
		send_res_header(conn.fd, NULL, NULL);

		// send file by chunks of 4069 bytes
		while (path_stat.st_size > CHUNK_SIZE) { /* if connection is closed the server will still send all the data, to a non existant connection
													TODO: find way to detect closed connection */
			sendfile(conn.fd, fd, NULL, CHUNK_SIZE);
			path_stat.st_size -= CHUNK_SIZE;
		}
		if (!fd_is_valid(conn.fd)) {
			printf("closed connection %d.\n", conn.fd);
		}
		sendfile(conn.fd, fd, NULL, path_stat.st_size);
		
		close(fd);
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
	int req_status = parse_req(conn.fd, &req);

	char *ip = inet_ntoa(conn.cli.sin_addr);

	char parsed_url[strlen(req.url) + 1];
	int url_status = parse_url(req.url, strlen(req.url), parsed_url);


	printf("%s:%d (fd: %d):\n\tmethod: %s\n\turl: %s (invalid: %d, parsed: %s)\n\tver: %s \n\tinvalid: %d\n",
			ip,
			conn.cli.sin_port,
			conn.fd,
			req.method,
			req.url,
			url_status,
			parsed_url,
			req.ver,
			req_status);
	printf("\theaders:\n");
	for (int i = 0; i < req.headers_len; i++) {
		printf("\t\t%s: %s\n", req.headers[i].name, req.headers[i].value);
	}

	if (req_status || url_status || check_url(parsed_url)) {
		send_res_status(conn.fd, "HTTP/1.1", 400, "Bad Request");
		
		send_res_header(conn.fd, "Server", "zerohttp");
		send_res_header(conn.fd, "Connection", "close");
		send_res_header(conn.fd, NULL, NULL);
	} else {
		serve_regular_request(conn, req, parsed_url);
	}

	free_req(req);
	close(conn.fd);
	printf("Closed connection %d.\n", conn.fd);
	
	return NULL;
}
