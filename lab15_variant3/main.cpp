#include <stdio.h>   
#include <stdlib.h>  
#include <stdint.h>  
#include <math.h>    
#include <string.h>  
#include <errno.h>   
#include <time.h>    
#include <pthread.h> 
#include <sys/stat.h>

static const char *CSV_HEADER =
    "timestamp,rows,cols,elements,points,threads,repeats,distance,input_mode,seed,"
    "serial_count,parallel_count,serial_ms,parallel_ms,speedup\n";

static FILE *open_results() {
    if (mkdir("results", 0755) != 0 && errno != EEXIST) {
        perror("Создание results");
        return NULL;
    }
    FILE *file = fopen("results/timings.csv", "w"); 
    if (file != NULL)
        fputs(CSV_HEADER, file);
    else
        perror("results/timings.csv");
    return file;
}

static int save_result(FILE *file, int rows, int cols, int threads, int repeats,
                       double distance, int manual, long long count,
                       double serial_ms, double parallel_ms) {
    FILE *history = fopen("results/runs.csv", "a+");
    if (history == NULL) {
        perror("results/runs.csv");
        return 0;
    }
    fseek(history, 0, SEEK_END);
    if (ftell(history) == 0)
        fputs(CSV_HEADER, history);
    time_t now = time(NULL);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S%z", localtime(&now));
    long long elements = (long long)rows * cols;
    const char *mode = manual ? "manual" : "generated";
    for (int i = 0; i < 2; i++) {
        FILE *destination = i == 0 ? file : history;
        fprintf(destination,
                "%s,%d,%d,%lld,%lld,%d,%d,%.17g,%s,%s,%lld,%lld,%.9f,%.9f,%.6f\n",
                timestamp, rows, cols, elements, elements / 2, threads, repeats,
                distance, mode, manual ? "" : "12345", count, count,
                serial_ms, parallel_ms, serial_ms / parallel_ms);
    }
    int ok = !ferror(file) && !ferror(history);
    if (fclose(history) != 0)
        ok = 0;
    if (!ok)
        fprintf(stderr, "Ошибка записи результатов.\n");
    return ok;
}

struct Shared {
    const double *matrix;
    int cols;
    double distance_squared;
    long long count;
    pthread_mutex_t mutex; 
};

struct Task {
    Shared *shared; 
    int begin;     
    int end;       
};


static long long count_rows(const double *a, int cols, int begin, int end,
                            double distance_squared) {
    long long count = 0;
    for (int i = begin; i < end; i++) {
        for (int j = 0; j < cols; j += 2) {
            double x = a[(size_t)i * cols + j];
            double y = a[(size_t)i * cols + j + 1];
            if (x * x + y * y > distance_squared)
                count++;
        }
    }
    return count;
}

static void *worker(void *argument) {
    Task *task = (Task *)argument; 
    Shared *s = task->shared;
    long long local = count_rows(s->matrix, s->cols, task->begin, task->end,
                                 s->distance_squared);
    pthread_mutex_lock(&s->mutex); 
    s->count += local;             
    pthread_mutex_unlock(&s->mutex);  
    return NULL;
}

static long long parallel_count(const double *a, int rows, int cols,
                                 int threads, double distance_squared) {
    pthread_t *ids = (pthread_t *)malloc((size_t)threads * sizeof(pthread_t));
    Task *tasks = (Task *)malloc((size_t)threads * sizeof(Task));
    if (ids == NULL || tasks == NULL) {
        fprintf(stderr, "Недостаточно памяти для потоков.\n");
        free(ids);
        free(tasks);
        return -1;
    }

    Shared shared;
    shared.matrix = a;
    shared.cols = cols;
    shared.distance_squared = distance_squared;
    shared.count = 0;
    int error = pthread_mutex_init(&shared.mutex, NULL);
    if (error != 0) {
        fprintf(stderr, "Ошибка mutex: %s\n", strerror(error));
        free(ids);
        free(tasks);
        return -1;
    }

    int created = 0;
    for (int k = 0; k < threads; k++) {
        tasks[k].shared = &shared;

        tasks[k].begin = (int)((long long)rows * k / threads);
        tasks[k].end = (int)((long long)rows * (k + 1) / threads);
        error = pthread_create(&ids[k], NULL, worker, &tasks[k]);
        if (error != 0) {
            fprintf(stderr, "Ошибка создания потока: %s\n", strerror(error));
            break;
        }
        created++;
    }

    for (int k = 0; k < created; k++) {
        // Не освобождаем матрицу, задачи и мьютекс, пока потоки работают.
        error = pthread_join(ids[k], NULL);
        if (error != 0) {
            fprintf(stderr, "Ошибка ожидания потока: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }
    }
    long long result = created == threads ? shared.count : -1;
    pthread_mutex_destroy(&shared.mutex);
    free(tasks);
    free(ids);
    return result;
}

static double seconds() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now); 
    return now.tv_sec + now.tv_nsec / 1000000000.0;
}

static int compare_double(const void *left, const void *right) {
    double a = *(const double *)left;
    double b = *(const double *)right;
    return (a > b) - (a < b);
}

static double median(double *times, int n) {
    qsort(times, n, sizeof(double), compare_double);
    if (n % 2 != 0)
        return times[n / 2];
    return (times[n / 2 - 1] + times[n / 2]) / 2;
}

static int measure(const double *a, int rows, int cols, int threads,
                    double distance, int repeats, double *serial_ms,
                    double *parallel_ms, long long *result) {
    double serial_times[101], parallel_times[101];
    double squared = distance * distance;
    long long expected = count_rows(a, cols, 0, rows, squared);
    
    if (parallel_count(a, rows, cols, threads, squared) != expected)
        return 0;

    for (int trial = 0; trial < repeats; trial++) {
        long long serial = -1, parallel = -1;
        for (int pass = 0; pass < 2; pass++) {
            double start = seconds();
            if ((trial + pass) % 2 == 0) {
                serial = count_rows(a, cols, 0, rows, squared);
                serial_times[trial] = (seconds() - start) * 1000;
            } else {
                parallel = parallel_count(a, rows, cols, threads, squared);
                parallel_times[trial] = (seconds() - start) * 1000;
            }
        }
        if (serial != expected || parallel != expected) {
            fprintf(stderr, "Результаты способов не совпали или поток не запустился.\n");
            return 0;
        }
    }

    *serial_ms = median(serial_times, repeats);
    *parallel_ms = median(parallel_times, repeats);
    *result = expected;
    return 1;
}

static double *create_matrix(int rows, int cols, int manual) {
    if ((size_t)rows > SIZE_MAX / sizeof(double) / (size_t)cols)
        return NULL;
    size_t elements = (size_t)rows * cols;
    double *a = (double *)malloc(elements * sizeof(double));
    if (a == NULL)
        return NULL;
    
    unsigned int state = 12345;
    if (manual)
        printf("Введите %zu чисел построчно (X Y X Y ...):\n", elements);
    for (size_t i = 0; i < elements; i++) {
        if (manual) {

            if (scanf("%lf", &a[i]) != 1 || !isfinite(a[i]) || fabs(a[i]) > 1e100) {
                fprintf(stderr, "Координата должна быть конечной, по модулю <= 1e100.\n");
                free(a);
                return NULL;
            }
        } else {
            state = state * 1664525u + 1013904223u;
            a[i] = (state % 20001) / 100.0 - 100.0; 
        }
    }
    return a;
}

static int integer(const char *text, int *value) {
    char *end;
    errno = 0;
    long number = strtol(text, &end, 10);
    if (errno || *text == '\0' || *end != '\0' || number < 1 || number > 100000000)
        return 0;
    *value = (int)number;
    return 1;
}

static int distance_value(const char *text, double *value) {
    char *end;
    errno = 0;
    *value = strtod(text, &end);
    return !errno && *text != '\0' && *end == '\0' && isfinite(*value)
           && *value >= 0 && *value <= 1e100;
}

static int benchmark(int threads, int repeats, double distance) {
    const int sizes[][2] = {{20, 20}, {100, 100}, {200, 1000}, {1000, 1000},
                            {2000, 1000}, {2000, 2000}, {4000, 4000}};
    FILE *csv = open_results();
    if (csv == NULL) {
        perror("results/timings.csv (сначала создайте каталог results)");
        return 1;
    }
    printf("Потоков: %d. Повторов: %d. Расстояние: %.6g.\n", threads, repeats, distance);
    printf("Матрица          Послед., мс  Паралл., мс  Ускорение\n");
    long long first_faster = 0;
    for (int i = 0; i < 7; i++) {

        int rows = sizes[i][0], cols = sizes[i][1];
        double *a = create_matrix(rows, cols, 0);
        double serial_ms, parallel_ms;
        long long count;
        
        if (a == NULL || !measure(a, rows, cols, threads, distance, repeats,
                                  &serial_ms, &parallel_ms, &count)) {
            free(a);
            fclose(csv);
            return 1;
        }
        free(a);

        long long elements = (long long)rows * cols;
        double speedup = serial_ms / parallel_ms;
        if (!save_result(csv, rows, cols, threads, repeats, distance, 0,
                         count, serial_ms, parallel_ms)) {
            fclose(csv);
            return 1;
        }
        
        printf("%5d x %-5d    %10.4f   %10.4f   %7.2f\n",
               rows, cols, serial_ms, parallel_ms, speedup);
        if (first_faster == 0 && parallel_ms < serial_ms)
            first_faster = elements;
    }
    
    if (fclose(csv) != 0) {
        perror("Запись CSV");
        return 1;
    }
    
    if (first_faster != 0)
        printf("Первый выигрыш среди проверенных размеров: %lld элементов.\n", first_faster);
    else
        printf("Среди проверенных размеров параллельный способ не оказался быстрее.\n");
    
    printf("Последний запуск: results/timings.csv. История: results/runs.csv.\n");
    printf("Обновить графики: make plot. Это измерение для текущей машины.\n");
    return 0;
}

int main(int argc, char **argv) {
    int rows = 1000, cols = 1000, threads = 4, repeats = 7, manual = 0;
    double distance = 50;
    if (argc >= 2 && strcmp(argv[1], "--benchmark") == 0) {
        if (argc > 5 || (argc >= 3 && !integer(argv[2], &threads))
            || (argc >= 4 && !integer(argv[3], &repeats))
            || (argc == 5 && !distance_value(argv[4], &distance))
            || threads > 20 || repeats > 101) {
            fprintf(stderr, "Использование: ./points --benchmark [потоков 1..20] [повторов 1..101] [расстояние]\n");
            return 1;
        }
        return benchmark(threads, repeats, distance);
    }
    
    int offset = 1;
    if (argc >= 2 && strcmp(argv[1], "--manual") == 0) {
        manual = 1;
        offset = 2;
    }
    
    if (argc != 1) {
        if ((argc != offset + 4 && argc != offset + 5) || !integer(argv[offset], &rows)
            || !integer(argv[offset + 1], &cols) || !integer(argv[offset + 2], &threads)
            || !distance_value(argv[offset + 3], &distance)
            || (argc == offset + 5 && !integer(argv[offset + 4], &repeats))
            || repeats > 101) {
            fprintf(stderr, "Использование: ./points [--manual] строки столбцы потоки расстояние [повторы 1..101]\n");
            return 1;
        }
    }
    
    if (cols % 2 != 0 || threads > rows || threads > 1024) {
        fprintf(stderr, "Столбцов должно быть чётное число; потоков <= строк и <= 1024.\n");
        return 1;
    }
    
    double *a = create_matrix(rows, cols, manual);
    if (a == NULL) {
        fprintf(stderr, "Не удалось создать матрицу.\n");
        return 1;
    }
    
    double serial_ms, parallel_ms;
    long long count;
    int ok = measure(a, rows, cols, threads, distance, repeats,
                     &serial_ms, &parallel_ms, &count);
    free(a); 
    
    if (!ok)
        return 1;
    
    FILE *csv = open_results();
    if (csv == NULL)
        return 1;

    int saved = save_result(csv, rows, cols, threads, repeats, distance, manual,
                            count, serial_ms, parallel_ms);
    if (fclose(csv) != 0 || !saved)
        return 1;
    
    printf("Матрица: %d x %d. Точек: %lld. Потоков: %d. Расстояние: %.6g.\n",
           rows, cols, (long long)rows * cols / 2, threads, distance);
    printf("Последовательный результат: %lld\nПараллельный результат: %lld\n", count, count);
    printf("Проверка: OK (результаты совпали во всех %d повторах).\n", repeats);
    printf("Медиана: последовательно %.6f мс, параллельно %.6f мс.\n", serial_ms, parallel_ms);
    printf("Ускорение: %.3f\n", serial_ms / parallel_ms);
    printf("Последний запуск: results/timings.csv. История: results/runs.csv.\n");
    printf("Обновить графики: make plot.\n");
    return 0;
}
