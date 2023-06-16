#include <string.h>

/* This function reads and discards data from fd. It's used to ignore the HTTP
 * response header. */
void waste_data(int fd);
int write_resilient(int fd, void *buff, size_t len);

static inline int dputs(int fd, char *str) {
	return write_resilient(fd, str, strlen(str));
}

#define STR_LITERAL(data) #data
#define STR(macro) STR_LITERAL(macro)
