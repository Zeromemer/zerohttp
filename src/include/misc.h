#ifndef HTTP_MISC_H
#define HTTP_MISC_H

#include <unistd.h>
#include <stdio.h>

int dgetc(int fd);

void *xmalloc(size_t size);
void xfree(void *ptr);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);
void *xreallocarray(void *ptr, size_t nmemb, size_t size);

#endif
