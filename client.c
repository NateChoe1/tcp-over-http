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

static int start_double_pipe(int argc, char **argv, int *recv, int *send);

int main(int argc, char **argv) {
	char ip_text[30];
	int send_fd, recv_fd;
	int source_fd, out_fd;
	struct pollfd fds[2];

	random_fd = open("/dev/urandom", O_RDONLY);
	if (random_fd < 0) {
		perror("Failed to open /dev/urandom");
		return 1;
	}

	if (create_proxy(CONFIG_HOST_IP, &send_fd, &recv_fd)) {
		fputs("Failed to create proxy\n", stderr);
		return 1;
	}

	if (start_double_pipe(argc - 1, argv + 1, &source_fd, &out_fd)) {
		fputs("Failed to start double pipe\n", stderr);
		return 1;
	}

	fds[0].fd = source_fd;
	fds[0].events = POLLIN;
	fds[1].fd = recv_fd;
	fds[1].events = POLLIN;

	for (;;) {
		poll(fds, 2, -1);
		if (fds[0].revents & POLLIN) {
			char buff[1024];
			ssize_t read_len;
			read_len = read(source_fd, buff, sizeof buff);
			if (read_len < 0) {
				perror("Warning: Failed to read from source");
				goto after_source;
			}
			if (write_resilient(send_fd, buff, read_len)) {
				fputs("Warning: Failed to send data\n",
						stderr);
			}
		}
after_source:
		if (fds[1].revents & POLLIN) {
			char buff[1024];
			ssize_t read_len;
			read_len = read(recv_fd, buff, sizeof buff);
			if (read_len < 0) {
				perror("Warning: Failed to recv from server");
				goto after_recv;
			}
			if (write_resilient(out_fd, buff, read_len)) {
				fputs("Warning: Failed to output data\n",
						stderr);
			}
		}
after_recv:
	}
}

static int create_proxy(char *ip_text, int *send_ret, int *recv_ret) {
	struct sockaddr_in addr;
	int send_fd, recv_fd;

	addr.sin_family = AF_INET;
	if (inet_aton(ip_text, &addr.sin_addr) == 0) {
		puts("Invalid address");
		return 0;
	}
	addr.sin_port = htons(CONFIG_PORT);

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
	if (write(send_fd, random_buff, 4) < 4) {
		perror("Failed to send random checksum");
		return 1;
	}

	if (dputsc(send_fd, "POST / HTTP/1.1\r\n"
			    "Host: " CONFIG_HOST "\r\n"
			    "Content-Length: 9223372036854775807\r\n"
			    "\r\n", random_buff)) {
		fputs("Failed to send HTTP header\n", stderr);
		return 1;
	}
	if (dputs(recv_fd, "GET / HTTP/1.1\r\n"
			   "Host: " CONFIG_HOST "\r\n"
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

static int start_double_pipe(int argc, char **argv, int *recv, int *send) {
	int pipe1[2], pipe2[2];
	pid_t pid;

	if (argc < 1) {
		fputs("Usage: client [double pipe command]\n", stderr);
		goto error1;
	}

	if (pipe(pipe1) == -1) {
		perror("Failed to create pipe");
		goto error1;
	}
	if (pipe(pipe2) == -1) {
		perror("Failed to create pipe");
		goto error2;
	}

	pid = fork();
	if (pid == -1) {
		perror("Failed to fork other process");
		goto error3;
	}
	if (pid == 0) {
		dup2(pipe1[1], 1);
		close(pipe1[0]);
		dup2(pipe2[0], 0);
		close(pipe2[1]);
		execvp(argv[0], argv);
		perror("exec() failed");
		exit(EXIT_FAILURE);
	}

	*recv = pipe1[0];
	close(pipe1[1]);
	*send = pipe2[1];
	close(pipe2[0]);

	return 0;

error3:
	close(pipe2[0]);
	close(pipe2[1]);
error2:
	close(pipe1[0]);
	close(pipe1[1]);
error1:
	return 1;
}
