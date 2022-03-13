#ifndef HTTP_H
#define HTTP_H

#include "tcp.h"
typedef struct {
	char *name;
	char *value;
} header_t;

typedef struct {
	char *name;
	char *value;
} query_selectors_t;

typedef struct {
	char *method;
	char *url;
	char *ver;
	header_t *headers;
	size_t headers_alloc_len;
	size_t headers_len;
} req_t;

enum http_req_err_t {
	NONE,
	URL,
	URL_LINE,
	HEADER,
	HEADER_SECTION,
	HEADER_FORMAT
};


int parse_url(char *input, size_t input_len, char *output, query_selectors_t **query_selectors, size_t *query_selectors_len);

int check_url(char *url);

int parse_req(int connfd, req_t *req);

char *get_header_value(header_t *headers, size_t len, char *query);

char *get_selector_value(query_selectors_t *query_selectors, size_t len, char *query);

int res_send_status(int connfd, char *ver, int status, char *msg);

#if defined(__GNUC__) || defined(__clang__)
__attribute__((__format__(__printf__, 3, 4))) // printf type checking
#endif
int res_send_headerf(int connfd, const char *header_name, const char *format, ...);

int res_send_end(int connfd);

void res_send_gmtime(conn_t conn);

void free_req(req_t req);

#endif
