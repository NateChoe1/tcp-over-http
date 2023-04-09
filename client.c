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

static int create_proxy(char *ip_text, int *send_ret, int *recv_ret);

static int dputs(int fd, char *str);
static int dputsc(int fd, char *str, char check[4]);

static int random_fd;

int main(int argc, char **argv) {
	char ip_text[30];
	int send_fd, recv_fd;
	struct pollfd fds[2];

	random_fd = open("/dev/urandom", O_RDONLY);
	if (random_fd < 0) {
		perror("Failed to open /dev/urandom");
		return 1;
	}

	if (create_proxy(CONFIG_SERVER_HOST, &send_fd, &recv_fd)) {
		fputs("Failed to create proxy\n", stderr);
		return 1;
	}

	dup2(send_fd, 1);
	dup2(recv_fd, 0);

	execlp(CONFIG_CLIENT_NC, CONFIG_CLIENT_NC, "-l", "-p",
			STR(CONFIG_CLIENT_PORT), NULL);
	perror("exec() failed");
	exit(EXIT_FAILURE);
}

static int create_proxy(char *ip_text, int *send_ret, int *recv_ret) {
	struct sockaddr_in addr;
	int send_fd, recv_fd;

	addr.sin_family = AF_INET;
	if (inet_aton(ip_text, &addr.sin_addr) == 0) {
		puts("Invalid address");
		return 0;
	}
	addr.sin_port = htons(CONFIG_SERVER_PORT);

	send_fd = socket(PF_INET, SOCK_STREAM, 0);
	recv_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (send_fd < 0 || recv_fd < 0) {
		perror("Failed to create a socket");
		return 1;
	}
	if (connect(send_fd, (struct sockaddr *) &addr, sizeof addr) == -1) {
		perror("Failed to connect send socket");
		return 1;
	}
	if (connect(recv_fd, (struct sockaddr *) &addr, sizeof addr) == -1) {
		perror("Failed to connect recv socket");
		return 1;
	}

	char random_buff[4], random_recv[4];
	if (read(random_fd, random_buff, 4) < 4) {
		perror("Failed to generate random checksum");
		return 1;
	}

	if (dputsc(send_fd, "POST / HTTP/1.1\r\n"
			    "Host: " CONFIG_HTTP_HOST "\r\n"
			    "Content-Length: 9223372036854775807\r\n"
			    "\r\n", random_buff)) {
		fputs("Failed to send HTTP header\n", stderr);
		return 1;
	}
	if (dputs(recv_fd, "GET / HTTP/1.1\r\n"
			   "Host: " CONFIG_HTTP_HOST "\r\n"
			   "\r\n")) {
		fputs("Failed to send HTTP header\n", stderr);
		return 1;
	}

	{
		char buff[1000];
		ssize_t buff_len;

		if ((buff_len = read(recv_fd, buff, sizeof buff)) < 0) {
			perror("Failed to receive header");
			return 1;
		}
		if (memcmp(random_buff, buff + buff_len - 4, 4)) {
			fputs("Received checksum doesn't match, quitting\n", stderr);
			return 1;
		}
	}

	*send_ret = send_fd;
	*recv_ret = recv_fd;
	return 0;
}

static int dputs(int fd, char *str) {
	return write_resilient(fd, str, strlen(str));
}

static int dputsc(int fd, char *str, char check[4]) {
	size_t len;
	int ret;
	char *buff;
	len = strlen(str);
	buff = malloc(len + 4);
	if (buff == NULL) {
		return 1;
	}
	memcpy(buff, str, len);
	memcpy(buff + len, check, 4);
	ret = write_resilient(fd, buff, len + 4);
	free(buff);
	return ret;
}
