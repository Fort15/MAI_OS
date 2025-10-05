#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        char msg[1024];
        uint32_t len = snprintf(msg, sizeof(msg) - 1, "error: need filename and string of floats\n");
        write(STDERR_FILENO, msg, len);
        exit(EXIT_FAILURE);
    }

	pid_t pid = getpid();

	int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (file == -1) {
		const char msg[] = "error: failed to open requested file\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

    char buf[4096];
    ssize_t bytes;
	while (bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1)) {
        if (bytes < 0) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        buf[bytes] = '\0';

        if (bytes > 0 && buf[bytes - 1] == '\n') {
            buf[bytes - 1] = '\0';
        }

        // парсинг
        float sum = 0;
        char *token = strtok(buf, " \t");
        while (token) {
            char *endptr;
            float f = strtof(token, &endptr);
            if (endptr != token && *endptr == '\0') { sum += f; }
            token = strtok(NULL, " \t");
        }
    
        char output[100];
        uint32_t len = snprintf(output, sizeof(output) - 1, "%.3f\n", sum);
        if (len < 0 || len >= sizeof(output)) {
            const char msg[] = "error: snprintf failed\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        

        if (write(file, output, len) != len) {
            const char msg[] = "error: failed to write to file\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }

	close(file);
    exit(EXIT_SUCCESS);
}