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

int send_res_status(int connfd, char *ver, int status, char *msg);

int send_res_header(int connfd, char *name, char *value);

int send_res_end(int connfd);

void send_res_gmtime(conn_t conn);

void free_req(req_t req);

#endif
