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
#include <pthread.h>
#include "include/req_handl.h"
#include "include/http.h"
#include "include/xmalloc.h"
#include "include/misc.h"
#include "include/mime.h"

#define CHUNK_SIZE 4096
#define SERVER "zerohttp"
#define MAX_REQUEST_SIZE 8192

char *srcs_dir = "./content";

void res_send_default(conn_t conn, int status, char *msg) {
	res_send_status(conn, "HTTP/1.1", status, msg);

	res_send_gmtime(conn);
	res_send_headerf(conn, "Server", SERVER);
	res_send_headerf(conn, "Connection", "close");
}

void serve_regular_request(conn_t conn, req_t req, char *parsed_url, query_selectors_t *query_selectors, size_t query_selectors_len) {
	if (strcmp(parsed_url, "/file_test") == 0) {
		if (strncmp(req.method, "POST", 4) != 0) {
			res_send_default(conn, 405, "Method Not Allowed");
			res_send_end(conn);

			return;
		}

		char *content_type = get_header_value(req.headers, req.headers_len, "Content-Type");
		if (content_type == NULL) {
			res_send_default(conn, 415, "Unsupported Media Type");
			res_send_end(conn);

			return;
		}

		if (strncmp(content_type, "text", 4) != 0) {
			res_send_default(conn, 415, "Unsupported Media Type");
			res_send_end(conn);

			return;
		}


		char *content_length = get_header_value(req.headers, req.headers_len, "Content-Length");
		if (!content_length) {
			res_send_default(conn, 411, "Length Required");
			res_send_end(conn);

			return;
		}
		int content_length_int = atoi(content_length);

		if (content_length_int > MAX_REQUEST_SIZE) {
			res_send_default(conn, 413, "Request Entity Too Large");
			res_send_end(conn);

			return;
		}

		char *body = xcalloc(content_length_int + 1, sizeof(char));
		if (recv(conn.fd, body, content_length_int, 0) != 0) {
			res_send_default(conn, 500, "Internal Server Error");
			res_send_end(conn);

			return;
		}
		
		body[content_length_int] = '\0';
		printf("%s\n", body);
		xfree(body);

		res_send_default(conn, 204, "No Content");
		res_send_end(conn);

		return;
	}

	if (strcmp(req.method, "GET") && strcmp(req.method, "HEAD")) {
		res_send_default(conn, 405, "Method Not Allowed");
		res_send_headerf(conn, "Allow", "GET, HEAD");
		res_send_end(conn);
		return;
	}

	if (strcmp(parsed_url, "/status") == 0) {
		res_send_default(conn, 200, "OK");
		res_send_headerf(conn, "Content-Type", "text/plain");
		res_send_end(conn);

		int fd = open("/proc/self/status", O_RDONLY);
		
		char buff[3072];
		int bytes_read = read(fd, buff, sizeof(buff));
		write(conn.fd, buff, bytes_read);
		
		close(fd);
		return;
	}
	
	char path[strlen(srcs_dir) + strlen(parsed_url) + sizeof("index.html")];

	strcat_mod(path, srcs_dir, parsed_url);

	struct stat path_stat;

	if (lstat(path, &path_stat) != -1 && S_ISDIR(path_stat.st_mode) && path[strlen(path) - 1] == '/') {
		strcat(path, "index.html");
	}

	if (lstat(path, &path_stat) != -1 && S_ISREG(path_stat.st_mode)) {
		int fd = open(path, O_RDONLY);

		if (fd == -1) {
			res_send_default(conn, 503, "Service Unavaliable");
			res_send_headerf(conn, "Content-Type", "text/plain");
			res_send_end(conn);

			dprintf(conn.fd, "Error: %s\n", strerror(errno));

			return;
		}

		res_send_default(conn, 200, "OK");
		char *download_sel = get_selector_value(query_selectors, query_selectors_len, "download");
		if (download_sel && strcmp(download_sel, "true") == 0) res_send_headerf(conn, "Content-Type", "application/octet-stream");
		else res_send_headerf(conn, "Content-Type", "%s",
							 (download_sel && strcmp(download_sel, "true") == 0) ?
							 "application/octet-stream" :
							 path_to_mime(path));
		res_send_headerf(conn, "Content-Length", "%ld", path_stat.st_size);
		res_send_end(conn);

		if (strcmp(req.method, "GET") == 0) {
			while (sendfile(conn.fd, fd, NULL, CHUNK_SIZE) == CHUNK_SIZE);
		}
		
		close(fd);
		return;
	}

	res_send_default(conn, 404, "Not Found");
	res_send_end(conn);
}

void serve_request(conn_t *conn_p) {
	conn_t conn = *(conn_t*)conn_p;
	xfree(conn_p);
	struct tm time_created;
	localtime_r(&conn.time_created, &time_created);
	req_t req = {0};
	int req_status = parse_req(conn, &req);

	char *ip = inet_ntoa(conn.data.sin_addr);

	if (req_status != 0) {
		res_send_default(conn, 400, "Bad Request");
		res_send_end(conn);
		dprintf(conn.fd, "Error: %s\n", http_strerror(req_status));
		close(conn.fd);
		return;
	}

	char *parsed_url = xcalloc(strlen(req.url) + 1, sizeof(char));
	query_selectors_t *query_selectors = NULL;
	size_t query_selectors_len = 0;
	int url_status = parse_url(req.url, strlen(req.url), parsed_url, &query_selectors, &query_selectors_len);

	if (url_status != 0) {
		res_send_default(conn, 400, "Bad Request");
		res_send_end(conn);
		dprintf(conn.fd, "Error: %s\n", http_strerror(url_status));
		close(conn.fd);
		return;
	}

	if (check_url(parsed_url) != 0) {
		res_send_default(conn, 400, "Bad Request");
		res_send_end(conn);
		dprintf(conn.fd, "Error: %s\n", "URL cannot contain ../");
		close(conn.fd);
		return;
	}


	printf("\033[32m->\033[0m [%02d:%02d:%02d] Opened connection %d: %s:%d: %s %s %s\n", time_created.tm_hour, time_created.tm_min, time_created.tm_sec,
		   conn.fd, ip, htons(conn.data.sin_port), req.method, req.url, req.ver);

	serve_regular_request(conn, req, parsed_url, query_selectors, query_selectors_len);

	free_req(req);
	xfree(query_selectors);
	xfree(parsed_url);
	close(conn.fd);
	struct tm time_closed;
	time_t current_time = time(NULL);
	localtime_r(&current_time, &time_closed);
	printf("\033[31m<-\033[0m [%02d:%02d:%02d] Closed connection %d.\n", time_closed.tm_hour, time_closed.tm_min, time_closed.tm_sec, conn.fd);
}
