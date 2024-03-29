#include "include/http.h"
#include "include/misc.h"
#include "include/xmalloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define SRH_BUFF_SIZE 8192

#define INITIAL_METHOD_SIZE 8
#define INITIAL_URL_SIZE 128
#define INITIAL_VER_SIZE 16
#define INITIAL_HEADER_SIZE 256

#define MAX_METHOD_SIZE 32
#define MAX_URL_SIZE 2048
#define MAX_VER_SIZE 16
#define MAX_HEADER_SIZE 2048

char *http_err_strings[] = {
	NULL,
	"Method too long",
	"URL too long",
	"Version too long",
	"Header too long",
	"Malformed URL",
	"Invalid URL line",
	"Invalid CRLF after header",
	"Invalid CRLF after headers section",
	"Incorrect header formating",
	"Invalid hex after '%' in url",
	"Equal sign after value in URL query",
	"URL query without a value",
};

char *http_strerror(int http_errnum) {
	return http_err_strings[http_errnum];
}

int parse_url(char *input, size_t input_len, char *output, query_selectors_t **query_selectors, size_t *query_selectors_len) {
	int output_prog = 0;
	
	for (int i = 0; i < input_len; i++) {
		if (input[i] == '?') {
			// exit loop if there is nothing after the '?'
			if (i == input_len - 1) {
				break;
			}

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
						return EXTRA_EQUAL_SIGN;
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
						(*query_selectors)[index].value = NULL;
					} else {
						return NO_VALUE_QUERY;
					}
				} else if (input[i] == '%') {
						if (i + 2 >= input_len)
							return INVALID_URL_HEX;

						char c = parse_hex_byte(input + i + 1);
						output[output_prog++] = c;
						i += 2;

						if (!c) return INVALID_URL_HEX;
				} else {
					output[output_prog++] = input[i];
				}

				i++;
			}

			if (!((*query_selectors)[index].value)) {
				return NO_VALUE_QUERY;
			}
			break;
		}

		if (input[i] == '%') {
			if (i + 2 >= input_len)
				return INVALID_URL_HEX;

			char c = parse_hex_byte(input + i + 1);
			output[output_prog++] = c;
			i += 2;

			if (!c) return INVALID_URL_HEX;
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

int parse_req(conn_t conn, req_t *req) {
	// get the method
	unsigned int size = INITIAL_METHOD_SIZE;
	unsigned int len = 0;
	req->method = xcalloc(size, 1);

	char c;
	while ((c = dgetc(conn.fd)) != ' ') {
		if (size > MAX_METHOD_SIZE) {
			free_req(*req);
			return METHOD_TOO_LONG;
		}
		req->method[len] = c;
		if (size == ++len) {
			size *= 2;
			req->method = xrealloc(req->method, size);
		}
	}
	req->method[len] = '\0';


	// get the url
	size = INITIAL_URL_SIZE;
	len = 0;
	req->url = xcalloc(size, 1);

	while ((c = dgetc(conn.fd)) != ' ') {
		if (size > MAX_URL_SIZE) {
			free_req(*req);
			return URL_TOO_LONG;
		}
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
	size = INITIAL_VER_SIZE;
	len = 0;
	req->ver = xcalloc(size, 1);

	while ((c = dgetc(conn.fd)) != '\r') {
		if (size > MAX_VER_SIZE) {
			free_req(*req);
			return VER_TOO_LONG;
		}
		req->ver[len] = c;
		if (size == ++len) {
			size *= 2;
			req->ver = xrealloc(req->ver, size);
		}
	}
	req->ver[len] = '\0';
	if (dgetc(conn.fd) != '\n') {
		return URL_LINE;
	}


	// get headers
	req->headers_len = 0;
	req->headers_alloc_len = 1;
	req->headers = xcalloc(req->headers_alloc_len , sizeof(header_t));

	for (;;) {
		if ((c = dgetc(conn.fd)) == '\r') {
			if (dgetc(conn.fd) == '\n')
				break;
			else {
				return HEADER_SECTION;
			}
		}

		unsigned int header_size = INITIAL_HEADER_SIZE;
		unsigned int header_len = 0;
		req->headers[req->headers_len].name = xcalloc(header_size, 1);
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

			if (header_size > MAX_HEADER_SIZE) {
				free_req(*req);
				return HEADER_TOO_LONG;
			}

			c = dgetc(conn.fd);
			if (c == '\r') {
				if (dgetc(conn.fd) == '\n')
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

int res_send_status(conn_t conn, char *ver, int status, char *msg) {
	return dprintf(conn.fd, "%s %d %s\r\n", ver, status, msg);
}

int res_send_headerf(conn_t conn, const char *header_name, const char *format, ...) {
	va_list args;
	char *buff = xcalloc(SRH_BUFF_SIZE, sizeof(char));
	char *prog = buff;

	va_start(args, format);

	prog += snprintf(prog, SRH_BUFF_SIZE - 2, "%s: ", header_name);
	prog += vsnprintf(prog, SRH_BUFF_SIZE - 2 - (prog - buff), format, args);
	prog[0] = '\r';
	prog[1] = '\n';
	prog += 2;

	write(conn.fd, buff, prog - buff);
	
	va_end(args);
	xfree(buff);
	return prog - buff;
}

int res_send_end(conn_t conn) {
	return dprintf(conn.fd, "\r\n");
}

void res_send_gmtime(conn_t conn) {
	struct tm gmt_tm;
	gmtime_r(&conn.time_created, &gmt_tm);
	char buff[sizeof("Thu, 01 Jan 1970 00:00:00")];
	strftime(buff, sizeof(buff), "%c", &gmt_tm);

	res_send_headerf(conn, "Date", "%s GMT", buff);
}

void free_req(req_t req) {
	xfree(req.method);
	xfree(req.url);
	xfree(req.ver);

	if (req.headers) {
		for (int i = 0; i < req.headers_len; i++) {
			xfree(req.headers[i].name);
		}
	}
	xfree(req.headers);
}