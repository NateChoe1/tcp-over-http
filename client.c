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
static int copy(int src, int dst);

int main(int argc, char **argv) {
	char ip_text[30];
	int serverfd;
	struct sockaddr_in addr;

	if ((serverfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Failed to create socket");
		return 1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CONFIG_CLIENT_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(serverfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
		perror("Failed to bind socket");
	}
	if (listen(serverfd, 5) == -1) {
		perror("Failed to listen on socket");
		return 1;
	}

	for (;;) {
		int clientfd;
		struct sockaddr_in addr_buff;
		socklen_t len_buff;
		pid_t pid;
		int forwardfd;
		clientfd = accept(serverfd,
				(struct sockaddr *) &addr_buff, &len_buff);
		if (clientfd < 0) {
			perror("Failed to accept connection");
			continue;
		}

		pid = fork();
		switch (pid) {
		case -1:
			perror("Failed to fork");
			close(clientfd);
			break;
		case 0:
			if (create_proxy(CONFIG_SERVER_HOST, &forwardfd)) {
				fputs("Failed to create proxy\n", stderr);
				return 1;
			}
			pid = fork();
			switch (pid) {
			case -1:
				perror("Failed to fork proxies");
				return 1;
			case 0:
				dup2(clientfd, 0);
				dup2(forwardfd, 1);
				execlp("cat", "cat", NULL);
				return 1;
			default:
				dup2(forwardfd, 0);
				dup2(clientfd, 1);
				execlp("cat", "cat", NULL);
				return 1;
			}
			break;
		default:
			close(clientfd);
			break;
		}
	}
	return 1;
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
