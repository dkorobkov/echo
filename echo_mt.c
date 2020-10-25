/*
 * A simple multithreaded echo server
 * Suitable for usage with reverse SSH to test if connection is alive
 * github.com/dkorobkov
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <pthread.h>

#define BUFFER_SIZE 1024 // Max incoming packet to be echoed

#define exit_on_error(...) { if(client_fd > 0) close(client_fd); fprintf(stderr, __VA_ARGS__); fflush(stderr); exit(1); }

void* echo_thread(void* param)
{
	int client_fd = (int) param;
	char buf[BUFFER_SIZE];
	int err;

//	fprintf(stderr, "Thread +\n");

	while (1)
	{
		int read = recv(client_fd, buf, BUFFER_SIZE, 0);

		if (!read) // Socket is closed on other end
		{
			close(client_fd);
			break;
		} // done reading

		if (read < 0)
			exit_on_error("Client read failed\n");

		err = send(client_fd, buf, read, 0);

		if (err < 0)
			exit_on_error("Client write failed\n");
	}

//	fprintf(stderr, "Thread -\n");

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s [port]\n", argv[0]);
		return 1;
	}

	int port = atoi(argv[1]);

	int server_fd, client_fd = -1, err;
	struct sockaddr_in server, client;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
		exit_on_error("Could not create socket\n");

	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	int opt_val = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

	err = bind(server_fd, (struct sockaddr *) &server, sizeof(server));
	if (err < 0)
		exit_on_error("Could not bind socket\n");

	err = listen(server_fd, 128);
	if (err < 0)
		exit_on_error("Could not listen on socket\n");

	fprintf(stderr, "Server is listening on %d\n", port);

	while (1)
	{
		socklen_t client_len = sizeof(client);
		client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);

		if (client_fd < 0)
			exit_on_error("Could not establish new connection\n");

		unsigned long ThreadId;
		pthread_attr_t attr;

		int res = pthread_attr_init(&attr);
		if (res != 0)
		{
			exit_on_error("pthread_attr_init returned error %d\n", res);
		}
		else
		{
			res = pthread_create(&ThreadId, &attr,
					echo_thread, (void*)(client_fd));
			if (res != 0)
			{
				exit_on_error("pthread_create returned error %d\n", res);
			}
			else
			{
				pthread_detach(ThreadId); // We do not want to pthreda_join after it finishes, just release resources
			}
		}
		pthread_attr_destroy(&attr);
	}

	return 0;
}
