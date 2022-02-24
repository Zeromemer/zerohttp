#include "include/http.h"
#include "include/misc.h"
#include "include/xmalloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// TODO: make a function that reads from a blocking file with a timeout

char *stat_1xx[] = {
	"Continue",
	"Switching Protocols",
	"Processing",
	"Early Hints"
};

char *stat_2xx[] = {
	"OK",
	"Created",
	"Accepted",
	"Non-Authoritative Information",
	"No Content",
	"Reset Content",
	"Partial Content",
	"Multi-Status",
};

char *stat_3xx[] = {
	"Multiple Choice",
	"Moved Permanently",
	"Found",
	"See Other",
	"Not Modified",
	NULL,
	NULL,
	"Temporary Redirect",
	"Permanent Redirect"
};

char *stat_4xx[] = {
	"Bad Request",
	"Unauthorized",
	"Payment Required",
	"Forbidden",
	"Not Found",
	"Method Not Allowed",
	"Not Acceptable",
	"Proxy Authentication Required",
	"Request Timeout",
	"Conflict",
	"Gone",
	"Length Required",
	"Precondition Failed",
	"Payload Too Large",
	"Unsupported Media Type",
	"Range Not Satisfiable",
	"Expectation Failed",
	"I'm a teapot",
	NULL,
	NULL,
	"Misdirected Request",
	"Unprocessable Entity",
	"Locked",
	"Failed Dependency",
	"Too Early",
	"Upgrade Required",
	"Precondition Required",
	"Too Many Requests",
	"Request Header Fields Too Large",
	"Unavailable For Legal Reasons"
};

char *stat_5xx[] = {
	"Unavailable For Legal Reasons",
	"Not Implemented",
	"Bad Gateway",
	"Service Unavailable",
	"Gateway Timeout",
	"HTTP Version Not Supported",
	"Variant Also Negotiates",
	"Insufficient Storage",
	"Loop Detected",
	NULL,
	"Not Extended",
	"Network Authentication Required"
};

char *stringify_status_code(int status) {
	int range = status / 100;
	int numbr = status % 100;

	char **range_arr;
	size_t range_arr_size;

	switch (range) {
	case 1: range_arr = stat_1xx; range_arr_size = STAT_ARR_SIZE(stat_1xx); break;
	case 2: range_arr = stat_2xx; range_arr_size = STAT_ARR_SIZE(stat_2xx); break;
	case 3: range_arr = stat_3xx; range_arr_size = STAT_ARR_SIZE(stat_3xx); break;
	case 4: range_arr = stat_4xx; range_arr_size = STAT_ARR_SIZE(stat_4xx); break;
	case 5: range_arr = stat_5xx; range_arr_size = STAT_ARR_SIZE(stat_5xx); break;
	default: return NULL;
	}

	if (numbr >= range_arr_size)
		return NULL;

	return range_arr[numbr];
}

static char *http_errs;

char *http_strerror() {
	return http_errs;
}

int parse_req(int connfd, req_t *req) {
	// getting the method and uri are not seperated into functions for performance

	// get the method
	req->method = xcalloc(1, 1);
	unsigned int size = 1;
	unsigned int len = 0;

	char c;
	while ((c = dgetc(connfd)) != ' ') {
		req->method[len] = c;
		if (size == ++len) {
			size *= 2;
			req->method = xrealloc(req->method, size);
		}
	}
	req->method[len] = '\0';

	// get the url
	req->url = xcalloc(1, 1);
	size = 1;
	len = 0;

	while ((c = dgetc(connfd)) != ' ') {
		req->url[len] = c;
		if (size == ++len) {
			size *= 2;
			req->url = xrealloc(req->url, size);
		}
	}
	req->url[len] = '\0';
	if (req->url[0] != '/') {
		http_errs = "Malformed URL";
		return 0;
	}

	// get http version
	req->ver = xmalloc(8);
	read(connfd, req->ver, 8);


	char crlf[2];
	read(connfd, crlf, 2);
	if (crlf[0] != '\r' || crlf[1] != '\n') { /* a crlf is expected after http version,
						     if there is none the request is invalid */
		http_errs = "No crlf after HTTP version";
		return 0;
	}


	req->headers = xmalloc(sizeof(header_t));
	req->headers_len = 0;
	req->headers_alloc_len = 1;

	
	// get headers
	for (;;) {
		// check if we've reached the end of the headers section
		if ((c = dgetc(connfd)) == '\r') {
			if (dgetc(connfd) == '\n')
				break;
			else {
				http_errs = "No crlf after ending of headers section";
				return 0;
			}
		}

		
		req->headers[req->headers_len].name = xmalloc(1);
		unsigned int header_size = 1;
		unsigned int header_len = 0;
		if (req->headers_alloc_len == req->headers_len + 1) {
			req->headers_alloc_len *= 2;
			req->headers = xreallocarray(req->headers, req->headers_alloc_len, sizeof(header_t));
		}



		for (;;) {
			req->headers[req->headers_len].name[header_len] = c;
			if (header_size == ++header_len) {
				header_size *= 2;
				req->headers[req->headers_len].name = realloc(
						req->headers[req->headers_len].name,
						header_size);
			}


			c = dgetc(connfd);
			if (c == '\r') {
				if (dgetc(connfd) == '\n')
					break;
				else {
					http_errs = "No crlf after ending of header";
					return 0;
				}
			}
		}
		req->headers[req->headers_len].name[header_len] = '\0';
		for (int i = 0; i < header_len; i++) {
			if (req->headers[req->headers_len].name[i] == ':') {
				if ((i + 2) > header_len) { /*The only ':' that was reached was at the end of
							      the header line, thus request is invalid*/
					http_errs = "Incorect header formating";
					return 0;
				}
				req->headers[req->headers_len].name[i] = '\0';
				req->headers[req->headers_len].value =
					req->headers[req->headers_len].name +
					i + 2;

				break;
			}
		}

		req->headers_len++;
	}





	return 1;
}

char *get_header_value(header_t *headers, size_t len, char *query) {
	for (int i = 0; i < len; i++) {
		if (!strcasecmp(headers[i].name, query)) {
			return headers[i].value;
		}
	}

	return NULL;
}

void send_res(int connfd, res_t res) {
	dprintf(connfd, "%s %d %s\r\n", res.ver, res.status, res.msg); // status line

	for (int i = 0; i < res.headers_len; i++) {
		dprintf(connfd, "%s: %s\r\n", res.headers[i].name, res.headers[i].value);
	} // headers
	
	dprintf(connfd, "\r\n"); // end headers section
}

void free_req(req_t req) {
	xfree(req.method);
	xfree(req.url);
	xfree(req.ver);

	for (int i = 0; i < req.headers_len; i++) {
		xfree(req.headers[i].name);
		
		// this is what I would do if name and value werent on the same memory block
		// free(req.headers[i].value);
	}
	xfree(req.headers);
}
