#include "include/misc.h"
#include <stdlib.h>
#include <stdio.h>

int dgetc(int fd) {
	char c;
	read(fd, &c, 1);

	return c;
}
