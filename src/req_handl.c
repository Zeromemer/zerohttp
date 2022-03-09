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

char *srcs_dir = "./req_src";

int reqs_count = 0;
pthread_mutex_t reqs_count_lock = PTHREAD_MUTEX_INITIALIZER;

void serve_regular_request(conn_t conn, req_t req, char *parsed_url, query_selectors_t *query_selectors, size_t query_selectors_len) {

	// check for dissalowed methods
	if (!(!strcmp(req.method, "GET") || !strcmp(req.method, "HEAD"))) {
		send_res_status(conn.fd, "HTTP/1.1", 405, "Method Not Allowed");
	
		send_res_headerf(conn.fd, "Server", SERVER);
		send_res_gmtime(conn);
		send_res_headerf(conn.fd, "Allow", "GET, HEAD");
		send_res_headerf(conn.fd, "Connection", "close");
		send_res_end(conn.fd);
		return;
	}

	char path[strlen(srcs_dir) + strlen(parsed_url) + sizeof("index.html")];

	strcat_mod(path, srcs_dir, parsed_url);

	if (!strcmp(parsed_url, "/status")) {
		send_res_status(conn.fd, "HTTP/1.1", 200, "OK");

		send_res_headerf(conn.fd, "Server", SERVER);
		send_res_gmtime(conn);
		send_res_headerf(conn.fd, "Content-Type", "text/plain");
		send_res_headerf(conn.fd, "Connection", "close");
		send_res_end(conn.fd);

		dprintf(conn.fd, "requests processed: %d\n", reqs_count);
		dprintf(conn.fd, "--- PROCESS INFORMATION ---\n\n");

		int fd = open("/proc/self/status", O_RDONLY);
		
		char buff[1024];
		int bytes_read = read(fd, buff, sizeof(buff));
		write(conn.fd, buff, bytes_read);
		

		close(fd);
		return;
	}


	struct stat path_stat;

	// if url points to dir append "index.html" at the end
	if (lstat(path, &path_stat) != -1 && S_ISDIR(path_stat.st_mode) && path[strlen(path) - 1] == '/') {
		strcat(path, "index.html");
	}

	// if file exists at path, send it
	if (lstat(path, &path_stat) != -1 && S_ISREG(path_stat.st_mode)) {
		int fd = open(path, O_RDONLY);
		char *mime_type = path_to_mime(path);

		send_res_status(conn.fd, "HTTP/1.1", 200, "OK");

		send_res_headerf(conn.fd, "Server", SERVER);
		send_res_gmtime(conn);
		char *download_sel = get_selector_value(query_selectors, query_selectors_len, "download");
		if (download_sel && !strcmp(download_sel, "true")) send_res_headerf(conn.fd, "Content-Type", "application/octet-stream");
		else send_res_headerf(conn.fd, "Content-Type", "%s", mime_type);
		send_res_headerf(conn.fd, "Content-Length", "%ld", path_stat.st_size);
		send_res_headerf(conn.fd, "Connection", "close");
		send_res_end(conn.fd);

		if (!strcmp(req.method, "GET")) {
			// send file by chunks of 4069 bytes
			while (sendfile(conn.fd, fd, NULL, CHUNK_SIZE) == CHUNK_SIZE);
		}
		
		close(fd);
		return;
	}

	// if this point is reached, a resource wasn't found, thus 404
	send_res_status(conn.fd, "HTTP/1.1", 404, "Not Found");
	send_res_gmtime(conn);
	send_res_headerf(conn.fd, "Server", SERVER);
	send_res_headerf(conn.fd, "Connection", "close");
	send_res_end(conn.fd);
}

void *serve_request(void *conn_p) {
	conn_t conn = *(conn_t*)conn_p;
	xfree(conn_p);
	struct tm time_created;
	localtime_r(&conn.time_created, &time_created);
	req_t req = {0};
	int req_status = parse_req(conn.fd, &req);

	char *ip = inet_ntoa(conn.cli.sin_addr);

	char *parsed_url = xcalloc(strlen(req.url) + 1, sizeof(char));
	query_selectors_t *query_selectors = NULL;
	size_t query_selectors_len = 0;
	int url_status = parse_url(req.url, strlen(req.url), parsed_url, &query_selectors, &query_selectors_len);


	printf("\033[32m->\033[0m [%02d:%02d:%02d] Opened connection %d: %s:%d: %s %s %s\n", time_created.tm_hour, time_created.tm_min, time_created.tm_sec,
	conn.fd, ip, conn.cli.sin_port, req.method, req.url, req.ver);

	if (req_status || url_status || check_url(parsed_url)) {
		send_res_status(conn.fd, "HTTP/1.1", 400, "Bad Request");
		
		send_res_headerf(conn.fd, "Server", SERVER);
		send_res_gmtime(conn);
		send_res_headerf(conn.fd, "Connection", "close");
		send_res_end(conn.fd);
	} else {
		serve_regular_request(conn, req, parsed_url, query_selectors, query_selectors_len);
		pthread_mutex_lock(&reqs_count_lock);
		reqs_count++;
		pthread_mutex_unlock(&reqs_count_lock);
	}

	free_req(req);
	xfree(query_selectors);
	xfree(parsed_url);
	close(conn.fd);
	printf("\033[31m<-\033[0m [%02d:%02d:%02d] Closed connection %d.\n", time_created.tm_hour, time_created.tm_min, time_created.tm_sec, conn.fd);
	
	pthread_exit(NULL);
}
