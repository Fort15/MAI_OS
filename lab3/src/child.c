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
#include <ctype.h>

#define SHM_SIZE 4096

const char SHM_NAME[] = "example_sh_memory";
const char SEM_NAME[] = "example_semaphore";

int main(int argc, char *argv[]) {
    if (argc != 2) {
        char msg[1024];
        uint32_t len = snprintf(msg, sizeof(msg) - 1, "error: need filename\n");
        write(STDERR_FILENO, msg, len);
        _exit(EXIT_FAILURE);
    }

	int shm = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm == -1 && errno != ENOENT) {
        const char msg[] = "error: failed to open SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (shm_buf == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(SEM_NAME, O_RDWR);
    if (sem == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphore\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    int file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (file == -1) {
        const char msg[] = "error: failed to open file";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    bool running = true;
    while (running) {
        sem_wait(sem);

        uint32_t *length = (uint32_t *)shm_buf;
        char *text = shm_buf + sizeof(uint32_t);

        if (*length == UINT32_MAX) {
            running = false;
        } else if (*length > 0 && *length < SHM_SIZE - sizeof(uint32_t)) {
            char local_buf[4096];
            memcpy(local_buf, text, *length);
            local_buf[*length] = '\0';
            *length = 0;

            sem_post(sem);

            float sum = 0.0f;
            char *token = strtok(local_buf, " \t");
            while (token) {
                char *endptr;
                float f = strtof(token, &endptr);
                if (endptr != token && (*endptr == '\0' || isspace(*endptr))) {
                    sum += f;
                }
                token = strtok(NULL, " \t");
            }

            char output[100];
            int len = snprintf(output, sizeof(output), "%.3f\n", sum);
            if (len > 0) {
                write(file, output, len);
            }

            continue;
        }
        
        sem_post(sem);
    }

	close(file);
    sem_close(sem);
    munmap(shm_buf, SHM_SIZE);
    exit(EXIT_SUCCESS);
}