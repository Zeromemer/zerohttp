#include "include/http.h"
#include "include/misc.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

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

	// get http version
	req->ver = xmalloc(8);
	read(connfd, req->ver, 8);

	char crlf[2];
	read(connfd, crlf, 2);
	if (crlf[0] != '\r' || crlf[1] != '\n') { /* a crlf is expected after http version,
						     if there is none the request is invalid */
		return 0;	
	}

	return 1;
}

void free_req(req_t req) {
	free(req.method);
	free(req.url);
	free(req.ver);
	for (int i = 0; i < req.headers_len; i++) {
		free(req.headers[i].name);
		free(req.headers[i].value);
	}
	free(req.headers);
	free(req.body);
}
