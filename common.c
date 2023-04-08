#include <stdio.h>

#include <errno.h>
#include <unistd.h>

#include "common.h"

void waste_data(int fd) {
	char buff[65536];
	if (read(fd, buff, sizeof buff) < 0) {
		fputs("Warning: waste read failed\n", stderr);
	}
}

int write_resilient(int fd, void *buff, size_t len) {
	char *start;
	start = buff;
	while (len > 0) {
		ssize_t written_len;
		written_len = write(fd, start, len);
		if (written_len < 0) {
			switch (errno) {
			case EBADF: case EWOULDBLOCK: case EDQUOT: case EFAULT:
			case EFBIG: case EINVAL: case EIO: case ENOSPC:
			case EPERM: case EPIPE:
				return 1;
			default:
				continue;
			}
		}
		else {
			len -= written_len;
			start += written_len;
		}
	}
	return 0;
}
