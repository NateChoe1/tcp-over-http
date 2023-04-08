#include <stdio.h>

#include <poll.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "config.h"
#include "common.h"

struct fd_pair {
	int send_fd;
	int recv_fd;
};

static void *run_sock(void *fds);

int main() {
	int sockfd;
	struct sockaddr_in addr;
	struct pollfd pollfd;

	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Failed to create socket");
		return 1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CONFIG_PORT);
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
		struct fd_pair fds;
		pthread_t thread;
		struct sockaddr_in addr_buff;
		socklen_t len_buff;

		/* We actually have no idea whether the first connection is a
		 * send or receive fd, so we just guess. */
		fds.recv_fd = accept(sockfd, (struct sockaddr *) &addr_buff,
				&len_buff);
		/* Make sure the second connection comes within 1000 ms. */
		poll(&pollfd, 1, 1000);
		if (!(pollfd.revents & POLLIN)) {
			close(fds.recv_fd);
			continue;
		}
		fds.send_fd = accept(sockfd, (struct sockaddr *) &addr_buff,
				&len_buff);

		pthread_create(&thread, NULL, run_sock, (void *) &fds);
	}

	return 0;
}

static void *run_sock(void *fds) {
	struct fd_pair *fd_pair;
	int dest_fd;
	struct sockaddr_in dest_addr;
	struct pollfd pollfds[2];

	fd_pair = (struct fd_pair *) fds;

	if ((dest_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Failed to create forwarding socket");
		goto end;
	}
	dest_addr.sin_family = AF_INET;
	if (inet_aton(CONFIG_DEST_IP, &dest_addr.sin_addr) == 0) {
		fputs("Invalid destination address\n", stderr);
		goto end;
	}
	dest_addr.sin_port = htons(CONFIG_DEST_PORT);
	if (connect(dest_fd, (struct sockaddr *) &dest_addr, sizeof dest_addr) == -1) {
		perror("Failed to connect to destination");
		goto end;
	}

	waste_data(fd_pair->send_fd);

	{
		char buff[1000];
		ssize_t header_len;
		header_len = read(fd_pair->recv_fd, buff, sizeof buff);
		if (header_len < 0) {
			goto end;
		}
		if (write(fd_pair->send_fd, buff + header_len - 4, 4) < 4) {
			perror("Sending checksum failed, quitting");
			goto end;
		}
	}

	pollfds[0].fd = fd_pair->recv_fd;
	pollfds[0].events = POLLIN;
	pollfds[1].fd = dest_fd;
	pollfds[1].events = POLLIN;

	for (;;) {
		char buff[1000];
		ssize_t data_len;

		poll(pollfds, 2, -1);

		if (pollfds[0].revents & POLLIN) {
			data_len = read(fd_pair->recv_fd, buff, sizeof buff);
			if (data_len < 0) {
				perror("Warning: Failed to read data, continuing");
				goto after_up;
			}
			if (write_resilient(dest_fd, buff, data_len)) {
				fputs("Failed to forward data\n", stderr);
				goto end;
			}
		}
after_up:
		if (pollfds[1].revents & POLLIN) {
			data_len = read(dest_fd, buff, sizeof buff);
			if (data_len < 0) {
				perror("Warning: Failed to receive from destination, continuing");
				goto after_down;
			}
			if (write_resilient(fd_pair->send_fd, buff, data_len)) {
				fputs("Failed to pass data back\n", stderr);
				goto end;
			}
		}
after_down:
	}

end:
	close(fd_pair->send_fd);
	close(fd_pair->recv_fd);
	return NULL;
}
