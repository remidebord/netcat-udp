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

int main(int argc, char **argv)
{
	char ibuffer[BUFFER_SIZE] = {0};
	char obuffer[BUFFER_SIZE] = {0};
	struct sockaddr_in saddr;
	struct pollfd pfd[3];
	int ilength, olength;
	int sock, port;
	char *ip = NULL;

	if ((argc == 1) || (argc > 3)) {
		printf("Usage: %s [HOST] PORT\n", argv[0]);
		return EXIT_FAILURE;
	} else if (argc == 3) {
		ip = argv[1];
		port = atoi(argv[2]);
	} else
		port = atoi(argv[1]);

	/* set network address, port */
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (ip)
		inet_pton(AF_INET, ip, &saddr.sin_addr);

	/* socket UDP */
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	/* bind socket and network infos */
	bind(sock, (struct sockaddr*)&saddr, sizeof(saddr));

	/* connect socket */
	if (connect(sock, (struct sockaddr*)&saddr, sizeof(saddr)) == -1)
		return EXIT_FAILURE;

	pfd[POLL_STDIN].fd = STDIN_FILENO;
	pfd[POLL_STDIN].events = POLLIN;

	pfd[POLL_STDOUT].fd = STDOUT_FILENO;
	pfd[POLL_STDOUT].events = 0;

	pfd[POLL_INET].fd = sock;
	pfd[POLL_INET].events = POLLIN;

	ilength = 0;
	olength = 0;

	while (1) {
		/* poll */
		if (poll(pfd, 3, -1) == -1) {
			printf("poll error (%d: %s).\n", errno, strerror(errno));
			return EXIT_FAILURE;
		}

		/* data from stdin ? */
		if ((pfd[POLL_STDIN].revents & POLLIN) && (ilength < sizeof(ibuffer))) {
			ilength = read(pfd[POLL_STDIN].fd, ibuffer, sizeof(ibuffer));
			if (ilength == -1) {
				printf("read error (%d: %s).\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}

			if (ilength > 0) {
				print_ascii(ibuffer, ilength);
				pfd[POLL_INET].events |= POLLOUT;
				pfd[POLL_STDIN].events = 0;
			}
		}

		/* write to network ? */
		if ((pfd[POLL_INET].revents & POLLOUT) && (ilength > 0)) {
			ilength = write(pfd[POLL_INET].fd, ibuffer, ilength);
			if (ilength == -1) {
				printf("write error (%d: %s).\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}

			if (ilength < sizeof(ibuffer))
				return EXIT_SUCCESS;

			ilength = 0;
		}

		/* read from network ? */
		if ((pfd[POLL_INET].revents & POLLIN) && (olength < sizeof(obuffer))) {
			printf("read sock\n");

			olength = read(pfd[POLL_INET].fd, obuffer, olength);
			if (olength == -1) {
				printf("read error (%d: %s).\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}

			if (olength > 0)
				print_ascii(obuffer, olength);

			/* enable stdout polling */
			if (olength > 0)
				pfd[POLL_STDOUT].events = POLLOUT;
			/* disable sock polling */
			if (olength == sizeof(obuffer))
				pfd[POLL_INET].events &= ~POLLIN;
		}

		if ((pfd[POLL_STDOUT].revents & POLLOUT) && (olength > 0)) {
			printf("write stdout\n");

			olength = write(pfd[POLL_STDOUT].fd, obuffer, olength);
			if (olength == -1) {
				printf("write error (%d: %s).\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}

			olength = 0;

			/* enable sock polling and disable stdout polling */
			if (olength < sizeof(obuffer)) {
				pfd[POLL_INET].events |= POLLIN;
				pfd[POLL_STDOUT].events = 0;
			}
		}
	}

	return EXIT_SUCCESS;
}

