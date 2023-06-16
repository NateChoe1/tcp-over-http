#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "config.h"
#include "common.h"

static int create_proxy(char *ip_text, int *fd_ret);

int main(int argc, char **argv) {
	char ip_text[30];
	int fd;

	if (create_proxy(CONFIG_SERVER_HOST, &fd)) {
		fputs("Failed to create proxy\n", stderr);
		return 1;
	}

	dup2(fd, 0);
	dup2(fd, 1);

	execlp(CONFIG_CLIENT_NC, CONFIG_CLIENT_NC, "-l", "-p",
			STR(CONFIG_CLIENT_PORT), NULL);
	perror("exec() failed");
	exit(EXIT_FAILURE);
}

static int create_proxy(char *ip_text, int *fd_ret) {
	struct sockaddr_in addr;
	int fd;
	char buff[1000];

	addr.sin_family = AF_INET;
	if (inet_aton(ip_text, &addr.sin_addr) == 0) {
		puts("Invalid address");
		return 0;
	}
	addr.sin_port = htons(CONFIG_SERVER_PORT);

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("Failed to create socket");
		return 1;
	}
	if (connect(fd, (struct sockaddr *) &addr, sizeof addr) == -1) {
		perror("Failed to connect send socket");
		return 1;
	}

	if (dputs(fd, "POST / HTTP/1.1\r\n"
		      "Host: " CONFIG_HTTP_HOST "\r\n"
		      "Content-Length: 9223372036854775807\r\n"
		      "\r\n")) {
		fputs("Failed to send HTTP header\n", stderr);
		return 1;
	}

	if (read(fd, buff, sizeof buff) < 0) {
		perror("Failed to receive header");
		return 1;
	}

	*fd_ret = fd;
	return 0;
}
