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
	unsigned int headers_len;
	char *body;
	unsigned int body_len;
} req_t;

typedef struct {
	conn_t conn;
	req_t req;
	int req_valid;
} client_t;

int parse_req(int connfd, req_t *req);

void free_req(req_t req);

#endif
