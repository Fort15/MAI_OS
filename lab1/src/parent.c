#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static char SERVER_PROGRAM_NAME[] = "child";


int main(int argc, char *argv[]) {
    // проверка аргументов
    if (argc == 1) {
        char msg[1024];
        uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
        write(STDERR_FILENO, msg, len);
        exit(EXIT_SUCCESS);
    }

    // путь до директории
    char progpath[1024];
	{
		ssize_t len = readlink("/proc/self/exe", progpath,
		                       sizeof(progpath) - 1);
		if (len == -1) {
			const char msg[] = "error: failed to read full program path\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}

		while (progpath[len] != '/')
			--len;

		progpath[len] = '\0';
	}

    // Parent => Child
    int client_to_server[2];
	if (pipe(client_to_server) == -1) {
		const char msg[] = "error: failed to create pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

    // НЕОБЯЗАТЕЛЬНО
    // // Child => Parent
    // int server_to_client[2]; 
	// if (pipe(server_to_client) == -1) {
	// 	const char msg[] = "error: failed to create pipe\n";
	// 	write(STDERR_FILENO, msg, sizeof(msg));
	// 	exit(EXIT_FAILURE);
	// }

    // дочерний процесс
    const pid_t child = fork();

    switch (child) {
	case -1: {
		const char msg[] = "error: failed to spawn new process\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	} break;

    case 0: { // дочерний
        {
            pid_t pid = getpid(); // получение PID дочернего

			char msg[64];
			const int32_t length = snprintf(msg, sizeof(msg),
				"%d: I'm a child\n", pid);
			write(STDOUT_FILENO, msg, length);
        }

        close(client_to_server[1]);

        dup2(client_to_server[0], STDIN_FILENO);
		close(client_to_server[0]);

        {
			char path[2048];
			snprintf(path, sizeof(path) - 1, "%s/%s", progpath, SERVER_PROGRAM_NAME);

			char *const args[] = {SERVER_PROGRAM_NAME, argv[1], NULL};

			int32_t status = execv(path, args);

			if (status == -1) {
				const char msg[] = "error: failed to exec into new exectuable image\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}
        }
    } break;

    default: { // PARENT
        {
            pid_t pid = getpid();

            char msg[64];
			const int32_t length = snprintf(msg, sizeof(msg),
				"%d: I'm a parent, my child has PID %d\n", pid, child);
			write(STDOUT_FILENO, msg, length);
        }

        close(client_to_server[0]);

        char buf[4096];
        ssize_t bytes = read(STDIN_FILENO, buf, sizeof(buf));
        if (bytes <= 0) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        write(client_to_server[1], buf, bytes);
        close(client_to_server[1]);

        wait(NULL);
    } break;
    }
}