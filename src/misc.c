#include "include/misc.h"
#include <stdlib.h>

int dgetc(int fd) {
	char c;
	read(fd, &c, 1);

	return c;
}
