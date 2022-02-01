#include "include/http.h"
#include "include/misc.h"
#include <stdlib.h>
#include <stdio.h>

request parse_req(int connfd) {
	request result = {0};

	// get the method
	result.method = malloc(1);
	unsigned int method_size = 1;
	unsigned int method_len = 0;

	char c;
	while ((c = dgetc(connfd)) != ' ') {
		result.method[method_len] = c;
		if (method_size == ++method_len) {
			method_size *= 2;
			result.method = reallocarray(result.method, 1, method_size);
		}
	}



	return result;
}
