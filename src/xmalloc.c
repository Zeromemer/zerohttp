#include "include/xmalloc.h"
#include <stdio.h>
#include <stdlib.h>

// TODO: make the xmalloc functions thread safe using semaphores

void *xmalloc(size_t size) {
	void *result = malloc(size);
	if (!result) {
		fprintf(stderr, "ERROR: malloc(%ld) returned NULL\n", size);
		exit(1);
	}
	return result;
}

void *xcalloc(size_t nmemb, size_t size) {
	void *result = calloc(nmemb, size);
	if (!result) {
		fprintf(stderr, "ERROR: calloc(%ld ,%ld) returned NULL\n", nmemb, size);
		exit(1);
	}
	return result;	
}

void *xrealloc(void *ptr, size_t size) {
	void *result = realloc(ptr, size);
	if (!result) {
		fprintf(stderr, "ERROR: realloc(%p, %ld) returned NULL\n", ptr, size);
		exit(1);
	}
	return result;
}

void *xreallocarray(void *ptr, size_t nmemb, size_t size) {
	void *result = reallocarray(ptr, nmemb, size);
	if (!result) {
		fprintf(stderr, "ERROR: reallocarray(%p, %ld, %ld) returned NULL\n", ptr, nmemb, size);
		exit(1);
	}
	return result;
}
