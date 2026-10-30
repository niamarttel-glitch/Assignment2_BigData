
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <psapi.h>

#define BLOCK_SIZE 32
#define REPETITIONS 3
#define SEED 123456789ULL

/* High-resolution timer for Windows */
double get_time_seconds(void) {
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

/* Reproducible pseudo-random number generator */
static uint64_t rng_state = SEED;

uint64_t next_random(void) {
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return rng_state * 2685821657736338717ULL;
}

double random_value(void) {
    int value = (int)(next_random() % 2001) - 1000;
    return value / 1000.0;
}

/* Allocate a square matrix */
double *allocate_matrix(size_t n) {
    if (n == 0 || n > SIZE_MAX / n ||
        n * n > SIZE_MAX / sizeof(double)) {
        return NULL;
    }

    return malloc(n * n * sizeof(double));
}

/* Baseline: i-j-k loop order */
void multiply_ijk(const double *A, const double *B,
                  double *C, size_t n) {
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            double sum = 0.0;

            for (size_t k = 0; k < n; k++) {
                sum += A[i * n + k] * B[k * n + j];
            }

            C[i * n + j] = sum;
        }
    }
}

/* Optimisation 1: i-k-j loop order */
void multiply_ikj(const double *A, const double *B,
                  double *C, size_t n) {
    for (size_t i = 0; i < n; i++) {
        for (size_t k = 0; k < n; k++) {
            double a = A[i * n + k];

            for (size_t j = 0; j < n; j++) {
                C[i * n + j] += a * B[k * n + j];
            }
        }
    }
}

/* Optimisation 2: cache blocking */
void multiply_blocked(const double *A, const double *B,
                      double *C, size_t n) {
    for (size_t ii = 0; ii < n; ii += BLOCK_SIZE) {
        for (size_t kk = 0; kk < n; kk += BLOCK_SIZE) {
            for (size_t jj = 0; jj < n; jj += BLOCK_SIZE) {

                size_t i_end = ii + BLOCK_SIZE < n
                               ? ii + BLOCK_SIZE : n;
                size_t k_end = kk + BLOCK_SIZE < n
                               ? kk + BLOCK_SIZE : n;
                size_t j_end = jj + BLOCK_SIZE < n
                               ? jj + BLOCK_SIZE : n;

                for (size_t i = ii; i < i_end; i++) {
                    for (size_t k = kk; k < k_end; k++) {
                        double a = A[i * n + k];

                        for (size_t j = jj; j < j_end; j++) {
                            C[i * n + j] +=
                                a * B[k * n + j];
                        }
                    }
                }
            }
        }
    }
}

/* Reset output matrix before timed multiplication */
void clear_matrix(double *C, size_t n) {
    memset(C, 0, n * n * sizeof(double));
}

/* Compare two matrices using absolute and relative tolerance */
int matrices_equal(const double *A, const double *B,
                   size_t n) {
    for (size_t i = 0; i < n * n; i++) {
        double difference = fabs(A[i] - B[i]);
        double tolerance = 1e-9 * (1.0 + fabs(A[i]));

        if (difference > tolerance) {
            return 0;
        }
    }

    return 1;
}

/* Sort measurements to calculate the median */
void sort_times(double *times, int count) {
    for (int i = 1; i < count; i++) {
        double current = times[i];
        int j = i - 1;

        while (j >= 0 && times[j] > current) {
            times[j + 1] = times[j];
            j--;
        }

        times[j + 1] = current;
    }
}

typedef void (*MultiplyFunction)(
    const double *, const double *, double *, size_t
);



double benchmark(MultiplyFunction multiply,
                 const double *A, const double *B,
                 double *C, size_t n, double *times) {
    clear_matrix(C, n);
    multiply(A, B, C, n);  /* Warm-up */

    for (int r = 0; r < REPETITIONS; r++) {
        clear_matrix(C, n);

        double start = get_time_seconds();
        multiply(A, B, C, n);
        double end = get_time_seconds();

        times[r] = end - start;
    }

    double sorted[REPETITIONS];

    for (int i = 0; i < REPETITIONS; i++) {
        sorted[i] = times[i];
    }

    sort_times(sorted, REPETITIONS);

    return sorted[REPETITIONS / 2];
}



void save_results(const char *method, size_t n,
                  const double *times, int count,
                  size_t peak_memory_bytes) {

    FILE *file = fopen("results/dense_results.csv", "a+");

    if (file == NULL) {
        fprintf(stderr, "Could not open results CSV.\n");
        return;
    }

    fseek(file, 0, SEEK_END);

    if (ftell(file) == 0) {
        fprintf(file,
                "size,method,repetition,time_seconds,"
                "peak_working_set_bytes,numeric_type,workers,"
                "warmup_repetitions,measured_repetitions\n");
    }

    for (int r = 0; r < count; r++) {
        fprintf(file,
                "%zu,%s,%d,%.9f,%zu,double,1,1,%d\n",
                n, method, r + 1, times[r],
                peak_memory_bytes, count);
    }

    fclose(file);
}

size_t get_peak_memory_bytes(void) {
    PROCESS_MEMORY_COUNTERS info;
    info.cb = sizeof(info);

    if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            &info,
            sizeof(info))) {
        return (size_t)info.PeakWorkingSetSize;
    }

    return 0;
}



int main(int argc, char *argv[]) {
    size_t n = 256;
    const char *method = "all";

    /* Read matrix size */
    if (argc > 1) {
        char *end;
        unsigned long long parsed = strtoull(argv[1], &end, 10);

        if (argv[1][0] == '\0' || *end != '\0' ||
            parsed == 0 || parsed > SIZE_MAX) {
            fprintf(stderr, "Invalid matrix size.\n");
            return 1;
        }

        n = (size_t)parsed;
    }

    /* Read selected method */
    if (argc > 2) {
        method = argv[2];
    }

    if (strcmp(method, "all") != 0 &&
        strcmp(method, "ijk") != 0 &&
        strcmp(method, "ikj") != 0 &&
        strcmp(method, "blocked") != 0) {
        fprintf(stderr,
                "Method must be all, ijk, ikj or blocked.\n");
        return 1;
    }

    /* Allocate matrices */
    double *A = allocate_matrix(n);
    double *B = allocate_matrix(n);
    double *C = allocate_matrix(n);
    double *reference = allocate_matrix(n);

    if (A == NULL || B == NULL || C == NULL ||
        reference == NULL) {
        fprintf(stderr,
                "Memory allocation failed for n=%zu.\n", n);
        free(A);
        free(B);
        free(C);
        free(reference);
        return 1;
    }

    /* Generate reproducible input matrices */
    rng_state = SEED;

    for (size_t i = 0; i < n * n; i++) {
        A[i] = random_value();
        B[i] = random_value();
    }

    double times[REPETITIONS];

    printf("Matrix size: %zu x %zu\n", n, n);
    printf("Numeric type: double\n");
    printf("Seed: %llu\n", (unsigned long long)SEED);
    printf("Method: %s\n", method);
    printf("Repetitions: %d plus one warm-up\n", REPETITIONS);

    int failed = 0;

    /*
     * Calculate the reference result with ijk.
     * This calculation is outside the timed measurements.
     */
    multiply_ijk(A, B, reference, n);

    /* Baseline: ijk */
    if (strcmp(method, "ijk") == 0 ||
        strcmp(method, "all") == 0) {

        double median = benchmark(
            multiply_ijk, A, B, C, n, times);

        if (!matrices_equal(C, reference, n)) {
            fprintf(stderr, "Validation FAILED for ijk.\n");
            failed = 1;
        } else {
            printf("Validation OK: ijk\n");
        }

        size_t peak_memory = get_peak_memory_bytes();

        printf("ijk median: %.6f seconds\n", median);
        printf("Peak working set: %.2f MiB\n",
               peak_memory / (1024.0 * 1024.0));

        save_results("ijk", n, times, REPETITIONS, peak_memory);
    }

    /* Optimisation 1: ikj */
    if (strcmp(method, "ikj") == 0 ||
        strcmp(method, "all") == 0) {

        double median = benchmark(
            multiply_ikj, A, B, C, n, times);

        if (!matrices_equal(C, reference, n)) {
            fprintf(stderr, "Validation FAILED for ikj.\n");
            failed = 1;
        } else {
            printf("Validation OK: ikj\n");
        }

        size_t peak_memory = get_peak_memory_bytes();

        printf("ikj median: %.6f seconds\n", median);
        printf("Peak working set: %.2f MiB\n",
               peak_memory / (1024.0 * 1024.0));

        save_results("ikj", n, times, REPETITIONS, peak_memory);
    }

    /* Optimisation 2: blocked */
    if (strcmp(method, "blocked") == 0 ||
        strcmp(method, "all") == 0) {

        double median = benchmark(
            multiply_blocked, A, B, C, n, times);

        if (!matrices_equal(C, reference, n)) {
            fprintf(stderr, "Validation FAILED for blocked.\n");
            failed = 1;
        } else {
            printf("Validation OK: blocked\n");
        }

        size_t peak_memory = get_peak_memory_bytes();

        printf("blocked median: %.6f seconds\n", median);
        printf("Peak working set: %.2f MiB\n",
               peak_memory / (1024.0 * 1024.0));

        save_results("blocked", n, times, REPETITIONS, peak_memory);
    }

    free(A);
    free(B);
    free(C);
    free(reference);

    return failed ? 1 : 0;
}
