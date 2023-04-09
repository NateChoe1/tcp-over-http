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

static void start_conn(int send_fd, int recv_fd);
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
		int send_fd, recv_fd;

		recv_fd = accept(sockfd, (struct sockaddr *) &addr_buff,
				&len_buff);
		/* Make sure the second connection comes within 1000 ms. */
		poll(&pollfd, 1, 1000);
		if (!(pollfd.revents & POLLIN)) {
			close(recv_fd);
			continue;
		}
		send_fd = accept(sockfd, (struct sockaddr *) &addr_buff,
				&len_buff);

		start_conn(send_fd, recv_fd);
	}

	return 0;
}

static void start_conn(int send_fd, int recv_fd) {
	pid_t pid;
	char buff[1000];
	ssize_t len;

	len = read(recv_fd, buff, sizeof buff);
	if (len < 0) {
		perror("Failed to start connection");
		return;
	}
	if (write(send_fd, buff + len - 4, 4) < 4) {
		perror("Failed to send checksum");
		return;
	}

	pid = fork();
	if (pid == -1) {
		return;
	}
	if (pid == 0) {
		dup2(send_fd, 1);
		dup2(recv_fd, 0);
		execlp(CONFIG_SERVER_NC, CONFIG_SERVER_NC, "--",
				CONFIG_DEST_HOST, STR(CONFIG_DEST_PORT), NULL);
		exit(EXIT_FAILURE);
	}

	close(send_fd);
	close(recv_fd);
}

static void sigchld_action(int signum) {
	wait(NULL);
}
