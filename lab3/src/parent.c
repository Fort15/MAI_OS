#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>
#include <stdio.h>

#define SHM_SIZE 4096

const char SHM_NAME[] = "example_sh_memory";
const char SEM_NAME[] = "example_semaphore";

static char SERVER_PROGRAM_NAME[] = "child";


int main(int argc, char *argv[]) {
    // проверка аргументов
    if (argc != 2) {
        char msg[1024];
        uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
        write(STDERR_FILENO, msg, len);
        _exit(EXIT_SUCCESS);
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

    // shared memory

    int shm = shm_open(SHM_NAME, O_RDWR, 0600);
    if (shm == -1 && errno != ENOENT) {
        const char msg[] = "error: failed to open SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (shm == -1) {
        const char msg[] = "error: failed to create SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    if (ftruncate(shm, SHM_SIZE) == -1) {
        const char msg[] = "error: failed to resize SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (shm_buf == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    // semaphore

    sem_t *sem = sem_open(SEM_NAME, O_RDWR | O_CREAT | O_TRUNC, 0600, 1);
    if (sem == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    // дочерний процесс
    const pid_t child = fork();

    switch (child) {
	case -1: {
		const char msg[] = "error: failed to spawn new process\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(EXIT_FAILURE);
	} break;

    case 0: { // дочерний
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
        
        bool running = 1;
        while (running) {
            char buf[SHM_SIZE - sizeof(uint32_t)];
            ssize_t bytes = read(STDIN_FILENO, buf, sizeof(buf));
            
            if (bytes < 0) {
                const char msg[] = "error: failed to read from stdin\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                _exit(EXIT_FAILURE);
            }

            sem_wait(sem);

            uint32_t *length = (uint32_t *)shm_buf;
            char *text = shm_buf + sizeof(uint32_t);

            if (bytes > 0) {
                *length = (uint32_t)bytes;
                memcpy(text, buf, bytes);
            } else {
                *length = UINT32_MAX;
                running = false;
            }
            sem_post(sem);
        }
    } break;
    }

    waitpid(child, NULL, 0);

    sem_close(sem);
    sem_unlink(SEM_NAME);
    munmap(shm_buf, SHM_SIZE);
    close(shm);
    shm_unlink(SHM_NAME);

    exit(EXIT_SUCCESS);
}