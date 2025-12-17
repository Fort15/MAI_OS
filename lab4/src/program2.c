#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include "libraries.h"


typedef int (*gcd_func)(int, int);
typedef int* (*sort_func)(int*, size_t);

enum CurrentLibrary {
    FIRST = 0,
    SECOND = 1
};


int command_0(const char **lib_names, void **library, int *current_lib,
             gcd_func *gcd, sort_func *sort) {

    dlclose(*library);

    switch (*current_lib) {
    case FIRST: {
        *current_lib = SECOND;
        break;
    }
    case SECOND: {
        *current_lib = FIRST;
        break;
    }
    default:
        return 1;
    }

    char buffer[1024];

    *library = dlopen(lib_names[*current_lib], RTLD_LAZY);
    if (!*library) {
        int length = snprintf(buffer, 1024, "Error: invalid switch libs; %s\n", dlerror());
        write(STDERR_FILENO, buffer, length);
        return 1;
    }

    *gcd = dlsym(*library, "gcd");
    if (!gcd)
    {
        const char msg[] = "Error: unable to get gcd function\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
    }

    *sort = dlsym(*library, "sort");
    if (!sort) {
        const char msg[] = "Error: unable to get sort fucntion\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
    }

    int length = snprintf(buffer, 1024, "Switched to %s library\n", lib_names[*current_lib]);
    write(STDOUT_FILENO, buffer, length);

    return 0;
}

void command_1(gcd_func gcd)
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

void command_2(sort_func sort)
{
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


int main() {
    const char *lib_names[] = {"./libd1.so", "./libd2.so"};
    int current_lib = FIRST;

    gcd_func gcd = NULL;
    sort_func sort = NULL;

    void *library = dlopen(lib_names[current_lib], RTLD_LAZY);
    char buffer[1024];
    if (!library) {
        int length = snprintf(buffer, 1024, "Error: unable to load lib; %s\n", dlerror());
        write(STDERR_FILENO, buffer, length);
        return 1;
    }

    gcd = dlsym(library, "gcd");
    if (!gcd) {
        const char msg[] = "Error: unable to get gcd function\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
    }

    sort = dlsym(library, "sort");
    if (!sort) {
        const char msg[] = "Error: unable to get sort function\n";
        write(STDERR_FILENO, msg, sizeof(msg)); 
    }

    {
        const char *msg = "Program 2.\nCommands: 0 (Switch) | 1 a b | 2 Size arg1...\n";
        write(STDOUT_FILENO, msg, strlen(msg));
    }

    int bytes_read;
    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *cmd_str = strtok(buffer, " \t\n");

        if (cmd_str == NULL) {
            continue;
        }

        int cmd = atoi(cmd_str);
        switch (cmd) {
        case 0: {
            int result = command_0(lib_names, &library, &current_lib, &gcd, &sort);
            if (result) {
                return result;
            }
            break;
        }
        case 1: {
            command_1(gcd);
            break;
        }
        case 2: {
            command_2(sort);
            break;
        }
        default:
            break;
        }
    }

    if (library) {
        dlclose(library);
    }

    return 0;
}

