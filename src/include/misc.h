#ifndef HTTP_MISC_H
#define HTTP_MISC_H

#define RESULT_ERROR (-1)

#include <stdio.h>

int dgetc(int fd);

size_t filesize(FILE* f); 

#endif
