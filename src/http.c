#include "include/http.h"
#include "include/misc.h"
#include "include/xmalloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

char *http_err_strings[6] = {
	NULL,
	"Malformed URL",
	"No crlf after URL line",
	"No crlf after header",
	"No crlf after ending of headers section",
	"Incorrect header formating"
};

char *http_strerror(int http_errnum) {
	return http_err_strings[http_errnum];
}

int parse_url(char *input, size_t input_len, char *output, query_selectors_t **query_selectors, size_t *query_selectors_len) {
	int output_prog = 0;
	
	for (int i = 0; i < input_len; i++) {
		if (input[i] == '?') {
			size_t alloc_size = 1;
			size_t index = 0;
			*query_selectors = xcalloc(alloc_size, sizeof(query_selectors_t));
			
			output[output_prog++] = '\0';
			i++;
			int nam_or_val = 0;
			*query_selectors_len = 1;
			(*query_selectors)[index].name = output + output_prog;
			while (i < input_len) {
				if (input[i] == '=') {
					if (!nam_or_val) {
						(*query_selectors)[index].value = output + output_prog + 1;
						nam_or_val = 1;
						output[output_prog++] = '\0';
					} else {
						return 1;
					}
				} else if (input[i] == '&') {
					if (nam_or_val) {
						nam_or_val = 0;
						output[output_prog++] = '\0';

						(*query_selectors_len)++;
						if (alloc_size == ++index) {
							alloc_size *= 2;
							*query_selectors = xreallocarray(*query_selectors, alloc_size, sizeof(query_selectors_t));
						}
						(*query_selectors)[index].name = output + output_prog;
					} else {
						return 1;
					}
				} else {
					output[output_prog++] = input[i];
				}

				i++;
			}

			output[output_prog++] = '\0';
			break;
		}

		if (input[i] == '%') {
			if (i + 2 >= input_len)
				return 1;

			char c = parse_hex_byte(input + i + 1);
			output[output_prog++] = c;
			i += 2;

			if (!c) return 1;
		} else {
			output[output_prog++] = input[i];
		}
	}
	output[output_prog++] = '\0';

	return 0;
}

int check_url(char *url) {
	int url_len = strlen(url);

	for (int i = 0; i < url_len - 2; i++) {
		if (url[i] == '/' && url[i+1] == '.' && url[i+2] == '.' && (url[i+3] == '/' || url[i+3] == '\0')) {
			return 1;
		}
	}

	return 0;
}

int parse_req(int connfd, req_t *req) { // TODO: make return value be an index of http_err_strings instead of http_errnum being set
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
		return URL;
	}

	// get http version
	req->ver = xmalloc(8);
	read(connfd, req->ver, 8);


	char crlf[2];
	read(connfd, crlf, 2);
	if (crlf[0] != '\r' || crlf[1] != '\n') { /* a crlf is expected after http version,
						     if there is none the request is invalid */
		return URL_LINE;
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
				return HEADER_SECTION;
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
				req->headers[req->headers_len].name = xrealloc(
						req->headers[req->headers_len].name,
						header_size);
			}


			c = dgetc(connfd);
			if (c == '\r') {
				if (dgetc(connfd) == '\n')
					break;
				else {
					return HEADER;
				}
			}
		}
		req->headers[req->headers_len].name[header_len] = '\0';
		int colon_reached = 0;
		for (int i = 0; i < header_len; i++) {
			if (req->headers[req->headers_len].name[i] == ':') {
				if ((i + 2) > header_len) { /*The only ':' that was reached was at the end of
							      the header line, thus request is invalid*/
					return HEADER_FORMAT;
				}
				colon_reached = 1;
				req->headers[req->headers_len].name[i] = '\0';
				req->headers[req->headers_len].value =
					req->headers[req->headers_len].name +
					i + 2;

				break;
			}
		}
		if (!colon_reached) { /*There is no colon at the end of the header line*/
			return HEADER_FORMAT;
		}

		req->headers_len++;
	}

	return 0;
}

char *get_header_value(header_t *headers, size_t len, char *query) {
	for (int i = 0; i < len; i++) {
		if (!strcasecmp(headers[i].name, query)) {
			return headers[i].value;
		}
	}

	return NULL;
}

char *get_selector_value(query_selectors_t *query_selectors, size_t len, char *query) {
	for (int i = 0; i < len; i++) {
		if (!strcmp(query_selectors[i].name, query)) {
			return query_selectors[i].value;
		}
	}

	return NULL;
}

void send_res_status(int connfd, char *ver, int status, char *msg) {
	dprintf(connfd, "%s %d %s\r\n", ver, status, msg);
}

void send_res_header(int connfd, char *name, char *value) {
	if (name) {
		dprintf(connfd, "%s: %s\r\n", name, value);
		return;
	}

	dprintf(connfd, "\r\n");
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
