#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include "libraries.h"


void func_1()
{
    char *arg1 = strtok(NULL, " \t\n");
    char *arg2 = strtok(NULL, " \t\n");

    size_t length = 0;
    char buffer[1024];

    if (arg1 && arg2) {
        int result = gcd(atoi(arg1), atoi(arg2));
        length = snprintf(buffer, 1024, "gcd(%d, %d) = %d\n", atoi(arg1), atoi(arg2), result);
        write(STDOUT_FILENO, buffer, length);
    }
}

void func_2() {
    char* size_str = strtok(NULL, " \t\n");
    if (!size_str) return;

    int size = atoi(size_str);
    int* array = (int*)malloc(size * sizeof(int));
    if (!array) {
        const char* message = "Malloc error\n";
        write(STDERR_FILENO, message, strlen(message));
        return;
    }

    for (int i = 0; i < size; ++i) {
        char* value = strtok(NULL, " \t\n");
        if (value) array[i] = atoi(value);
        else array[i] = 0;
    }

    sort(array, size);

    {
        const char* message = "Sorted array: ";
        write(STDOUT_FILENO, message, strlen(message));
    }

    char buffer[1024];
    for (int i = 0; i < size; ++i) {
        int len = snprintf(buffer, 1024, "%d ", array[i]);
        write(STDOUT_FILENO, buffer, len);
    }

    write(STDOUT_FILENO, "\n", 1);
    free(array);
}

int main(void) {
    {
        const char* message = "Program 1.\nCommands: 1 a b | 2 Size arg1...\n";
        write(STDOUT_FILENO, message, strlen(message));
    }

    int bytes = 0;
    char buffer[1024];

    while ((bytes = read(STDIN_FILENO, buffer, 1024 - 1)) > 0) {
        buffer[bytes] = '\0';

        char *token = strtok(buffer, " \t\n");
        if (!token) {
            continue;
        }

        int func = atoi(token);
        switch (func) {
        case 1: {
            func_1();
            break;
        }

        case 2: {
            func_2();
            break;
        }

        default:
            break;
        }
    }

    return 0;
}