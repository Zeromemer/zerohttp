#include "include/misc.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>

int char_value(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}

	return -1;
}

int dgetc(int fd) {
	char c;
	read(fd, &c, 1);

	return c;
}

ssize_t read_timeout(int fd, void *buff, size_t count, time_t tv_sec, suseconds_t tv_usec) {
	struct timeval tv;
	tv.tv_sec = tv_sec;
	tv.tv_usec = tv_usec;

	fd_set rfds;


	ssize_t ret = select(fd+1, &rfds, NULL, NULL, &tv);

	if (ret && FD_ISSET(fd, &rfds)) {
		ret = read(fd, buff, count);
	}

	return ret;
}

char parse_hex_byte(char *byte_buff) {
	int first_val = char_value(byte_buff[0]) * 16;
	int second_val = char_value(byte_buff[1]);

	return ((first_val < 0) || (second_val < 0)) ? 0 : first_val + second_val;
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
