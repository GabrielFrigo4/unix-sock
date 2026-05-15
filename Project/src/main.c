#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "api.h"
#include "http.h"
#include "main.h"
#include "server.h"

/* ── Definições e Estado ─────────────────────────────────── */

constexpr size_t PIPE_FD_COUNT = 2;
constexpr size_t POLL_EVENT_COUNT = 2;

static int sig_pipe[PIPE_FD_COUNT];

/* ── Gerenciamento de Sinais (Self-Pipe) ──────────────────── */

static void generic_signal_handler(const int signum)
{
	const int saved_errno = errno;
	(void)!write(sig_pipe[1], &signum, sizeof(int));
	errno = saved_errno;
}

static int setup_self_pipe(void)
{
	if (pipe(sig_pipe) == -1)
	{
		perror("[ERRO]: pipe");
		return -1;
	}

	const int flags_read = fcntl(sig_pipe[0], F_GETFL, 0);
	fcntl(sig_pipe[0], F_SETFL, flags_read | O_NONBLOCK);

	const int flags_write = fcntl(sig_pipe[1], F_GETFL, 0);
	fcntl(sig_pipe[1], F_SETFL, flags_write | O_NONBLOCK);

	const struct sigaction sa = {.sa_handler = generic_signal_handler, .sa_flags = SA_RESTART};

	if (sigaction(SIGINT, &sa, nullptr) == -1 || sigaction(SIGTSTP, &sa, nullptr) == -1 ||
	    sigaction(SIGCHLD, &sa, nullptr) == -1)
	{
		perror("[ERRO]: sigaction");
		return -1;
	}

	return sig_pipe[0];
}

static void process_signals(const int pipe_read_fd, bool *const running)
{
	int signum;
	while (read(pipe_read_fd, &signum, sizeof(int)) == sizeof(int))
	{
		if (signum == SIGCHLD)
		{
			int status;
			pid_t pid;
			while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
			{
				printf("[INFO]: Zombie reaped (PID: %d)\n", (int)pid);
			}
			continue;
		}

		if (signum == SIGINT || signum == SIGTSTP)
		{
			printf("\n[INFO]: Shutdown signal received...\n");
			*running = false;
		}
	}
}

/* ── Ciclo de Vida do Processo ───────────────────────────── */

static void handle_child_process(const int server_fd, const int client_socket)
{
	close(server_fd);
	close(sig_pipe[0]);
	close(sig_pipe[1]);
	http_handle_client(client_socket);
	close(client_socket);
	_exit(EXIT_SUCCESS);
}

static void accept_and_fork(const int server_fd)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	const int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

	if (client_socket < 0)
	{
		perror("[ERRO]: accept");
		return;
	}

	printf("[INFO]: New socket created: %d\n", client_socket);

	const pid_t pid = fork();

	if (pid < 0)
	{
		close(client_socket);
		perror("[ERRO]: fork");
		return;
	}

	if (pid == 0)
	{
		handle_child_process(server_fd, client_socket);
	}

	close(client_socket);
}

/* ── Entry Point (Processo Pai) ──────────────────────────── */

int main(void)
{
	const int pipe_read_fd = setup_self_pipe();
	if (pipe_read_fd == -1)
	{
		return EXIT_FAILURE;
	}

	const int server_fd = server_init(IP, PORT, IP_MODE_DUAL_STACK);
	struct pollfd fds[POLL_EVENT_COUNT] = {
	    {.fd = server_fd, .events = POLLIN, .revents = 0},
	    {.fd = pipe_read_fd, .events = POLLIN, .revents = 0}
	};

	bool running = true;
	api_init();

	while (running)
	{
		const int poll_result = poll(fds, POLL_EVENT_COUNT, -1);

		if (poll_result < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			perror("[ERRO]: Erro fatal no poll");
			break;
		}

		if (fds[1].revents & POLLIN)
		{
			process_signals(pipe_read_fd, &running);
		}

		if (fds[0].revents & POLLIN)
		{
			accept_and_fork(server_fd);
		}
	}

	printf("[INFO]: Closing server socket and exiting...\n");
	close(server_fd);
	close(sig_pipe[0]);
	close(sig_pipe[1]);

	return EXIT_SUCCESS;
}
