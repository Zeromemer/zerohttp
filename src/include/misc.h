#ifndef HTTP_MISC_H
#define HTTP_MISC_H

#define RESULT_ERROR (-1)

#include <stdio.h>

#define STAT_ARR_SIZE(arr) sizeof(arr) / sizeof(arr[0])

int dgetc(int fd);

size_t filesize(FILE* f); 

#endif
