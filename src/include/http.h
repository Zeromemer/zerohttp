#ifndef HTTP_H
#define HTTP_H

typedef struct {
	char *name;
	char *value;
} header;

typedef struct {
	char *method;
	char *url;
	char *ver;
	header *headers;
	unsigned int headers_len;
	char *body;
	unsigned int body_len;
} request;

int parse_req(int connfd, request *req);

#endif
