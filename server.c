#include <stdio.h>
#include <stdlib.h>

#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "config.h"
#include "common.h"

static void start_conn(int fd);
static void sigchld_action(int signum);

int main() {
	int sockfd;
	struct sockaddr_in addr;
	struct pollfd pollfd;
	struct sigaction act;
	sigset_t sigset;

	sigemptyset(&sigset);
	act.sa_handler = sigchld_action;
	act.sa_mask = sigset;
	act.sa_flags = 0;
	if (sigaction(SIGCHLD, &act, NULL) == -1) {
		perror("Warning: sigaction() failed, defunct processes will "
				"spawn");
	}

	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Failed to create socket");
		return 1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CONFIG_SERVER_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof addr) == -1) {
		perror("Failed to bind socket");
		return 1;
	}
	if (listen(sockfd, 2) == -1) {
		perror("Failed to listen on socket");
		return 1;
	}

	pollfd.fd = sockfd;
	pollfd.events = POLLIN;

	for (;;) {
		struct sockaddr_in addr_buff;
		socklen_t len_buff;
		int client_fd;

		client_fd = accept(sockfd, (struct sockaddr *) &addr_buff,
				&len_buff);

		start_conn(client_fd);
		close(client_fd);
	}

	return 0;
}

static void start_conn(int fd) {
	pid_t pid;
	char buff[1000];

	/* Swallow header */
	if (read(fd, buff, sizeof buff) < 0) {
		perror("Failed to start connection");
		return;
	}
	if (dputs(fd, "HTTP/1.1 500 Internal Server Error\r\n"
		      "Content-Length: 9223372036854775807\r\n"
		      "\r\n")) {
		fputs("Failed to send response header\n", stderr);
		return;
	}


	pid = fork();
	if (pid == -1) {
		return;
	}
	if (pid == 0) {
		dup2(fd, 1);
		dup2(fd, 0);
		execlp(CONFIG_SERVER_NC, CONFIG_SERVER_NC, "--",
				CONFIG_DEST_HOST, STR(CONFIG_DEST_PORT), NULL);
		exit(EXIT_FAILURE);
	}
}

static void sigchld_action(int signum) {
	wait(NULL);
}
