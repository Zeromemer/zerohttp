#ifndef XMALLOC_H
#define XMALLOC_H
#include <stddef.h>

void *xmalloc_inter(size_t size, char *file, int line);
void xfree_inter(void *ptr, char *file, int line);
void *xcalloc_inter(size_t nmemb, size_t size, char *file, int line);
void *xrealloc_inter(void *ptr, size_t size, char *file, int line);
void *xreallocarray_inter(void *ptr, size_t nmemb, size_t size, char *file, int line);


#define xmalloc(size) xmalloc_inter(size, __FILE__, __LINE__)
#define xfree(ptr) xfree_inter(ptr, __FILE__, __LINE__)
#define xcalloc(nmemb, size) xcalloc_inter(nmemb, size, __FILE__, __LINE__)
#define xrealloc(ptr, size) xrealloc_inter(ptr, size, __FILE__, __LINE__)
#define xreallocarray(ptr, nmemb, size) xreallocarray_inter(ptr, nmemb, size, __FILE__, __LINE__)

#endif
