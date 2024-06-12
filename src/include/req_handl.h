#ifndef REQ_HANDL_H
#define REQ_HANDL_H

#include "http.h"
#include "main.h"

typedef struct {
	arguments_t arguments;
	conn_t conn;
} request_params_t;

void serve_request(request_params_t *request_params);

#endif
