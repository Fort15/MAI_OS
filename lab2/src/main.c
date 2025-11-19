#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>


sem_t thread_limit_sem;

typedef struct {
    const int *arr;     
    size_t start_idx;   
    size_t end_idx;     
    int local_min;   
    int local_max;      
} ThreadArgs;

void *worker_thread(void *raw_args) {
    ThreadArgs *args = (ThreadArgs *)raw_args;

    if (sem_wait(&thread_limit_sem) != 0) {
        perror("Ошибка sem_wait в потоке");
        return NULL;
    }

    args->local_min = args->local_max = args->arr[args->start_idx];

    for (size_t i = args->start_idx + 1; i < args->end_idx; ++i) {
        if (args->arr[i] < args->local_min) args->local_min = args->arr[i];
        if (args->arr[i] > args->local_max) args->local_max = args->arr[i];
    }

    if (sem_post(&thread_limit_sem) != 0) {
        perror("Ошибка sem_post в потоке");
        return NULL;
    }

    return NULL;
}


int *generate_large_array(size_t n, size_t *size_out) {
    int *arr = malloc(n * sizeof(int));
    if (!arr) { *size_out = 0; return NULL; }
    int A = -1e9, B = 1e9;
    for (size_t i = 0; i < n; ++i) {
        arr[i] = A + rand() % (B - A + 1);
    }
    *size_out = n;
    return arr;
}

double sequential_version(const int *arr, size_t n, int *p_min, int *p_max) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    *p_min = *p_max = arr[0];
    for (size_t i = 1; i < n; ++i) {
        if (arr[i] < *p_min) *p_min = arr[i];
        if (arr[i] > *p_max) *p_max = arr[i];
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    return (end.tv_sec - start.tv_sec) * 1000.0 +
        (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

double parallel_version(const int* arr, size_t n, size_t K_threads, size_t L_limit, int* p_min, int* p_max) {
    struct timespec start, end;

    if (sem_init(&thread_limit_sem, 0, (unsigned int)L_limit) != 0) {
        perror("Ошибка инициализации семафора");
        return -1.0;
    }

    size_t part_size = n / K_threads;

    pthread_t *threads = malloc(K_threads * sizeof(pthread_t));
    ThreadArgs *thread_args = malloc(K_threads * sizeof(ThreadArgs));

    if (!threads || !thread_args) {
        perror("Ошибка выделения памяти");
        sem_destroy(&thread_limit_sem);  
        free(threads);
        free(thread_args);
        return -1.0;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (size_t i = 0; i < K_threads; ++i) {
        size_t start_idx = i * part_size;
        size_t end_idx = (i == K_threads - 1) ? n : (i + 1) * part_size;

        thread_args[i].arr = arr;
        thread_args[i].start_idx = start_idx;
        thread_args[i].end_idx = end_idx;

        if (pthread_create(&threads[i], NULL, worker_thread, &thread_args[i]) != 0) {
            perror("Ошибка создания потока.");
            // ждём уже запущенные потоки
            for (size_t j = 0; j < i; ++j) {
                pthread_join(threads[j], NULL);
            }

            sem_destroy(&thread_limit_sem);
            free(threads);
            free(thread_args);
            return -1.0;
        }
    }

    // ждём завершения всех потоков
    for (size_t i = 0; i < K_threads; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("ошибка pthread_join");
        }
    }

    *p_min = thread_args[0].local_min;
    *p_max = thread_args[0].local_max;

    for (size_t i = 1; i < K_threads; ++i) {
        if (thread_args[i].local_min < *p_min) *p_min = thread_args[i].local_min;
        if (thread_args[i].local_max > *p_max) *p_max = thread_args[i].local_max;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    sem_destroy(&thread_limit_sem);
    free(thread_args);
    free(threads);
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1000000.0;
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <размер_массива_N> <число_потоков_K> <лимит_потоков_L>\n", argv[0]);
        return EXIT_FAILURE;
    }

    size_t N = atoi(argv[1]);
    size_t K_threads = atoi(argv[2]); 
    size_t L_limit = atoi(argv[3]);

    if (N < 3 || K_threads == 0 || L_limit == 0 || L_limit > K_threads) {
        fprintf(stderr, "Ошибка: N>=3, K>0, L>0, L<=K\n");
        return EXIT_FAILURE;
    }


    size_t arr_size = 0;
    int *arr = generate_large_array(N, &arr_size);
    if (!arr || arr_size == 0) {
        fprintf(stderr, "Ошибка при создании массива чисел\n");
        return EXIT_FAILURE;
    }


    // Последовательная версия
    int min_seq, max_seq;
    double t_seq = sequential_version(arr, arr_size, &min_seq, &max_seq);
    if (t_seq < 0) {
        fprintf(stderr, "Ошибка в последовательной версии\n");
        free(arr);
        return EXIT_FAILURE;
    }

    // Параллельная версия
    int min_par, max_par;
    double t_par = parallel_version(arr, arr_size, K_threads, L_limit, &min_par, &max_par);
    if (t_par < 0) {
        fprintf(stderr, "Ошибка в параллельной версии\n");
        free(arr);
        return EXIT_FAILURE;
    }


    printf("N=%zu\n", N);
    printf("K=%zu (всего потоков)\n", K_threads);
    printf("L=%zu (макс. одновременно работающих)\n", L_limit);
    printf("t_seq (последовательно): %.3f мс\n", t_seq);
    printf("t_par (параллельно):    %.3f мс\n", t_par);
    printf("Ускорение S = t_seq/t_par: %.3f\n", t_seq / t_par);
    printf("Эффективность E = S/K: %.3f\n", (t_seq / t_par) / K_threads);

    if (min_seq == min_par && max_seq == max_par) {
        printf("Найдено (min, max) = (%d, %d)\n", min_seq, max_seq);
    } else {
        printf("ОШИБКА: Sequential (%d, %d) != Parallel (%d, %d).\n", min_seq, max_seq, min_par, max_par);
    }
    free(arr);
    return EXIT_SUCCESS;
}