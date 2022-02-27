#ifndef HTTP_H
#define HTTP_H

#include "tcp.h"
typedef struct {
	char *name;
	char *value;
} header_t;

typedef struct {
	char *method;
	char *url;
	char *ver;
	header_t *headers;
	size_t headers_alloc_len;
	size_t headers_len;
} req_t;

typedef struct {
	char *ver;
	int status;
	char *msg;
	header_t *headers;
	size_t headers_alloc_len;
	size_t headers_len;
} res_t;

int parse_url(char *input, size_t input_len, char *output);

int parse_req(int connfd, req_t *req);

char *get_header_value(header_t *headers, size_t len, char *query);

void send_res(int connfd, res_t res);

char *stringify_status_code(int status);

void free_req(req_t req);

#endif
