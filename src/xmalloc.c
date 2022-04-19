#include "include/xmalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <malloc.h>

// #define XMALLOC_DEBUG

void *xmalloc_inter(size_t size, char *file, int line) {
	void *result = malloc(size);
	if (!result) {
		fprintf(stderr, "%s:%d: malloc(%ld) returned NULL\n", file, line, size);
		raise(SIGTRAP);
		exit(1);
	}
	
	#ifdef XMALLOC_DEBUG
	printf("%s:%d: xmalloc %p\n", file, line, result);
	#endif
	return result;
}

void xfree_inter(void *ptr, char *file, int line) {
	#ifdef XMALLOC_DEBUG
	printf("%s:%d: xfree %p\n", file, line, ptr);
	#endif
	free(ptr);
}

void *xcalloc_inter(size_t nmemb, size_t size, char *file, int line) {
	void *result = calloc(nmemb, size);
	if (!result) {
		fprintf(stderr, "%s:%d: calloc(%ld ,%ld) returned NULL\n", file, line, nmemb, size);
		raise(SIGTRAP);
		exit(1);
	}
	
	#ifdef XMALLOC_DEBUG
	printf("%s:%d: xcalloc %p\n", file, line, result);
	#endif
	return result;	
}

void *xrealloc_inter(void *ptr, size_t size, char *file, int line) {
	void *result = realloc(ptr, size);
	if (!result && size) { // realloc(NULL, 0) returns NULL, but we don't want to treat that as an error
		fprintf(stderr, "%s:%d: realloc(%p, %ld) returned NULL\n", file, line, ptr, size);
		raise(SIGTRAP);
		exit(1);
	}
	
	#ifdef XMALLOC_DEBUG
	printf("%s:%d: xrealloc %p -> %p\n", file, line, ptr, result);
	#endif
	return result;
}

void *xreallocarray_inter(void *ptr, size_t nmemb, size_t size, char *file, int line) {
	void *result = reallocarray(ptr, nmemb, size);
	if (!result && nmemb && size) { // reallocarray(NULL, 0, 0) returns NULL, but we don't want to treat that as an error
		fprintf(stderr, "%s:%d: reallocarray(%p, %ld, %ld) returned NULL\n", file, line, ptr, nmemb, size);
		raise(SIGTRAP);
		exit(1);
	}

	#ifdef XMALLOC_DEBUG
	printf("%s:%d: xreallocarray %p -> %p\n", file, line, ptr, result);
	#endif
	return result;
}
