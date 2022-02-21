#include "include/misc.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int dgetc(int fd) {
	char c;
	read(fd, &c, 1);

	return c;
}

size_t filesize(FILE *f) {
	if (f == NULL)
		return RESULT_ERROR;

	if (fseek(f, 0, SEEK_END) < 0)
		return RESULT_ERROR;


	size_t length = ftell(f);
	fseek(f, 0, SEEK_SET);
	return length;
}
