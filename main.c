#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> /* read() */
#include <string.h>
#include <errno.h>
#include <poll.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define POLL_STDIN	0
#define POLL_STDOUT	1
#define POLL_INET	2

#define BUFFER_SIZE 32

struct pollfd pfd[3];

void print_ascii(char *buffer, int length)
{
	for (int i = 0; i < length; i++) {
		if (buffer[i] > 0x20 && buffer[i] < 0x7F)
			printf("%c", buffer[i]);
		else
			printf("\\%02x", buffer[i]);
	}

	printf("\n");
}

int client(char* ip, int port)
{
	char buffer[BUFFER_SIZE] = {0};
	struct sockaddr_in saddr;
	int sock, length;

#ifdef DEBUG
	printf("start client...\n");
#endif

	/* set network address, port */
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &saddr.sin_addr);

	/* socket UDP */
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	/* bind socket to destination IP address */
	if (connect(sock, (struct sockaddr*)&saddr, sizeof(saddr)) == -1) {
		printf("connect error (%d: %s).\n", errno, strerror(errno));
		return EXIT_FAILURE;
	}

	pfd[POLL_INET].fd = sock;
	pfd[POLL_INET].events = 0;

	length = 0;

	while (1) {
		/* configure events to poll */
		if (poll(pfd, 3, -1) == -1) {
			printf("poll error (%d: %s).\n", errno, strerror(errno));
			return EXIT_FAILURE;
		}

		/* read from stdin ? */
		if (pfd[POLL_STDIN].revents & POLLIN) {
			length = read(pfd[POLL_STDIN].fd, buffer, sizeof(buffer));
			if (length == -1) {
				printf("read error (%d: %s).\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}

			if (length > 0) {
#ifdef DEBUG
				print_ascii(buffer, length);
#endif
				/* enable write to network */
				pfd[POLL_INET].events = POLLOUT;
				/* disable read from stdin */
				pfd[POLL_STDIN].events = 0;
			}
		}

		/* write to network ? */
		if ((pfd[POLL_INET].revents & POLLOUT) && (length > 0)) {
			length = write(pfd[POLL_INET].fd, buffer, length);
			if (length == -1) {
				printf("write error (%d: %s).\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}

			/* no more data to send ? */
			if (length < sizeof(buffer))
				return EXIT_SUCCESS;
			else {
				/* disable write to network */
				pfd[POLL_INET].events = 0;
				/* enable read from stdin */
				pfd[POLL_STDIN].events = POLLIN;
			}

			length = 0;
		}
	}
}

int server(char* ip, int port)
{
	char buffer[BUFFER_SIZE] = {0};
	struct sockaddr_in saddr;
	int sock, length;

#ifdef DEBUG
	printf("start server...\n");
#endif

	/* set network address, port */
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (ip)
		inet_pton(AF_INET, ip, &saddr.sin_addr);

	/* socket UDP */
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	/* bind socket to port */
	bind(sock, (struct sockaddr*)&saddr, sizeof(saddr));

	pfd[POLL_INET].fd = sock;
	pfd[POLL_INET].events = POLLIN;

	length = 0;

	while (1) {
		/* configure events to poll */
		if (poll(pfd, 3, -1) == -1) {
			printf("poll error (%d: %s).\n", errno, strerror(errno));
			return EXIT_FAILURE;
		}

		/* read from network ? */
		if (pfd[POLL_INET].revents & POLLIN) {
#ifdef DEBUG
			printf("read sock\n");
#endif
			length = read(pfd[POLL_INET].fd, buffer, sizeof(buffer));
			if (length == -1) {
				printf("read error (%d: %s).\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}

			if (length > 0) {
#ifdef DEBUG
				print_ascii(buffer, length);
#endif
				/* enable write to stdout */
				pfd[POLL_STDOUT].events = POLLOUT;
				/* disable read from network */
				pfd[POLL_INET].events = 0;
			}

			pfd[POLL_INET].revents = 0;
		}

		/* write to stdout ? */
		if ((pfd[POLL_STDOUT].revents & POLLOUT) && (length > 0)) {
#ifdef DEBUG
			printf("write stdout\n");
#endif
			if (write(pfd[POLL_STDOUT].fd, buffer, length) == -1) {
				printf("write error (%d: %s).\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}

			/* enable read from network */
			pfd[POLL_INET].events = POLLIN;
			/* disable stdout write */
			pfd[POLL_STDOUT].events = 0;
			/* clear length */
			length = 0;

			pfd[POLL_STDOUT].revents = 0;
		}
	}
}

void usage(int argc, char **argv)
{
	printf("Usage: %s [HOST] PORT\n", argv[0]);
}

int main(int argc, char **argv)
{
	char *ip = NULL;
	int port;

	if ((argc == 1) || (argc > 3)) {
		printf("Usage: %s [HOST] PORT\n", argv[0]);
		return EXIT_FAILURE;
	} else if (argc == 3) {
		ip = argv[1];
		port = atoi(argv[2]);
	} else
		port = atoi(argv[1]);

	pfd[POLL_STDIN].fd = STDIN_FILENO;
	pfd[POLL_STDIN].events = POLLIN;

	if (poll(pfd, 1, 100) == -1) {
		printf("poll error (%d: %s).\n", errno, strerror(errno));
		return EXIT_FAILURE;
	}

	/* data available on stdin ? */
	if (pfd[POLL_STDIN].revents & POLLIN) {
		if (ip)
			return client(ip, port);
		else {
			usage(argc, argv);
			return EXIT_FAILURE;
		}
	}

	return server(ip, port);
}

