#include "include/http.h"
#include "include/misc.h"
#include "include/xmalloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static char* http_errs;

char* http_strerror() {
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
	size = 1;


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
		if (size == req->headers_len + 1) {
			size *= 2;
			req->headers = xreallocarray(req->headers, size, sizeof(header_t));
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

void free_req(req_t req) {
	free(req.method);
	free(req.url);
	free(req.ver);
	for (int i = 0; i < req.headers_len; i++) {
		free(req.headers[i].name);
		
		// this is what I would do if name and value werent on the same memory block
		// free(req.headers[i].value);
	}
	free(req.headers);
	free(req.body);
}
