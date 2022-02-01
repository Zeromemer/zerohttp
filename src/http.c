#include "include/http.h"
#include "include/misc.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int parse_req(int connfd, request *req) {
	// getting the method and uri are not seperated into functions for performance

	// get the method
	req->method = malloc(1);
	unsigned int size = 1;
	unsigned int len = 0;

	char c;
	while ((c = dgetc(connfd)) != ' ') {
		req->method[len] = c;
		if (size == ++len) {
			size *= 2;
			req->method = reallocarray(req->method, 1, size);
		}
	}

	// get the uri
	req->uri = malloc(1);
	size = 1;
	len = 0;

	while ((c = dgetc(connfd)) != ' ') {
		req->uri[len] = c;
		if (size == ++len) {
			size *= 2;
			req->uri = reallocarray(req->uri, 1, size);
		}
	}

	// get http version
	req->ver = malloc(8);
	read(connfd, req->ver, 8);

	char crlf[2];
	read(connfd, crlf, 2);
	if (crlf[0] != '\r' || crlf[1] != '\n') { /* a crlf is expected after http version,
						     if there is none request is invalid */
		return 0;

		
	}

	return 1;
}
