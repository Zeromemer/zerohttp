#ifndef HTTP_H
#define HTTP_H

typedef struct {
	char* name;
	char* value;
} header;

typedef struct {
	char* method;
	char* uri;
	char* ver;
	header* headers;
	unsigned int headers_len;
	char* body;
	unsigned int body_len;
} request;

request parse_req(int connfd);

#endif
