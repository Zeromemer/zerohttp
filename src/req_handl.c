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
#include <time.h>
#include "include/req_handl.h"
#include "include/http.h"
#include "include/xmalloc.h"
#include "include/misc.h"
#include "include/mime.h"

#define CHUNK_SIZE 4096

char *srcs_dir = "./req_src";
char *server = "zerohttp";

void serve_regular_request(conn_t conn, req_t req, char *parsed_url, query_selectors_t *query_selectors, size_t query_selectors_len) {
	if (!(!strcmp(req.method, "GET") || !strcmp(req.method, "HEAD"))) {
		send_res_status(conn.fd, "HTTP/1.1", 405, "Method Not Allowed");
	
		send_res_header(conn.fd, "Server", server);
		send_res_header(conn.fd, "Connection", "close");
		send_res_header(conn.fd, NULL, NULL);
		return;
	}

	char path[strlen(srcs_dir) + strlen(parsed_url)];

	strcat_mod(path, srcs_dir, parsed_url);


	struct stat path_stat;
	if (lstat(path, &path_stat) != -1 && S_ISREG(path_stat.st_mode)) {
		int fd = open(path, O_RDONLY);
		char *mime_type = path_to_mime(path);

		char content_length_s[sizeof("-2147483648")];
		sprintf(content_length_s, "%ld", path_stat.st_size);

		send_res_status(conn.fd, "HTTP/1.1", 200, "OK");

		send_res_header(conn.fd, "Server", server);
		char *download_sel = get_selector_value(query_selectors, query_selectors_len, "download");
		if (download_sel && !strcmp(download_sel, "true")) send_res_header(conn.fd, "Content-Type", "application/octet-stream");
		else send_res_header(conn.fd, "Content-Type", mime_type);
		send_res_header(conn.fd, "Content-Length", content_length_s);
		send_res_header(conn.fd, "Connection", "keep-alive");
		send_res_header(conn.fd, NULL, NULL);

		if (!strcmp(req.method, "GET")) {
			// send file by chunks of 4069 bytes
			while (path_stat.st_size > CHUNK_SIZE) { /* if connection is closed the server will still send all the data, to a non existant connection
														TODO: find way to detect closed connection */
				sendfile(conn.fd, fd, NULL, CHUNK_SIZE);
				path_stat.st_size -= CHUNK_SIZE;
			}
			sendfile(conn.fd, fd, NULL, path_stat.st_size);
		}
		
		close(fd);
		return;
	}

	send_res_status(conn.fd, "HTTP/1.1", 404, "Not Found");
	
	send_res_header(conn.fd, "Server", server);
	send_res_header(conn.fd, "Connection", "close");
	send_res_header(conn.fd, NULL, NULL);
}

void *serve_request(void *conn_p) {
	conn_t conn = *(conn_t*)conn_p;
	xfree(conn_p);
	struct tm time_created;
	localtime_r(&conn.time_created, &time_created);
	req_t req = {0};
	int req_status = parse_req(conn.fd, &req);

	char *ip = inet_ntoa(conn.cli.sin_addr);

	char parsed_url[strlen(req.url) + 1];
	query_selectors_t *query_selectors;
	size_t query_selectors_len = 0;
	int url_status = parse_url(req.url, strlen(req.url), parsed_url, &query_selectors, &query_selectors_len);


	printf("\033[32m->\033[0m [%02d:%02d:%02d] %s:%d (fd: %d): %s %s %s\n", time_created.tm_hour, time_created.tm_min, time_created.tm_sec,
	ip, conn.cli.sin_port, conn.fd, req.method, req.url, req.ver);

	if (req_status || url_status || check_url(parsed_url)) {
		send_res_status(conn.fd, "HTTP/1.1", 400, "Bad Request");
		
		send_res_header(conn.fd, "Server", server);
		send_res_header(conn.fd, "Connection", "close");
		send_res_header(conn.fd, NULL, NULL);
	} else {
		serve_regular_request(conn, req, parsed_url, query_selectors, query_selectors_len);
	}

	free_req(req);
	close(conn.fd);
	printf("\033[31m<-\033[0m [%02d:%02d:%02d] Closed connection %d.\n", time_created.tm_hour, time_created.tm_min, time_created.tm_sec, conn.fd);
	
	return NULL;
}
