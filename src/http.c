#include "include/http.h"
#include "include/misc.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

request parse_req(int connfd) {
	// getting the method and uri are not seperated into functions for performance


	request result = {0};

	// get the method
	result.method = malloc(1);
	unsigned int size = 1;
	unsigned int len = 0;

	char c;
	while ((c = dgetc(connfd)) != ' ') {
		result.method[len] = c;
		if (size == ++len) {
			size *= 2;
			result.method = reallocarray(result.method, 1, size);
		}
	}

	// get the uri
	result.uri = malloc(1);
	size = 1;
	len = 0;

	while ((c = dgetc(connfd)) != ' ') {
		result.uri[len] = c;
		if (size == ++len) {
			size *= 2;
			result.uri = reallocarray(result.uri, 1, size);
		}
	}

	// get http version
	result.ver = malloc(8);
	read(connfd, result.ver, 8);



	return result;
}
