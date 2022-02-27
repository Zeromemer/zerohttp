#ifndef HTTP_MISC_H
#define HTTP_MISC_H

#define RESULT_ERROR (-1)

#include <stdio.h>
#include <sys/time.h>

#define STAT_ARR_SIZE(arr) sizeof(arr) / sizeof(arr[0])

int dgetc(int fd);

int dgetc_timeout(int fd, time_t tv_sec, suseconds_t tv_usec);

int is_hex(char c);

char parse_hex_byte(char *byte_buff);

size_t filesize(FILE* f); 

#endif
