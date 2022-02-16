#ifndef XMALLOC_H
#define XMALLOC_H
#include <stddef.h>

void *xmalloc(size_t size);
void xfree(void *ptr);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);
void *xreallocarray(void *ptr, size_t nmemb, size_t size);

#endif
