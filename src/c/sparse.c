
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <psapi.h>

#define REPETITIONS 3
#define SEED 123456789ULL

typedef struct {
    size_t rows;
    size_t cols;
    size_t nnz;
    double *values;
    size_t *col_indices;
    size_t *row_ptr;
} CSRMatrix;

static uint64_t rng_state = SEED;

static uint64_t next_random(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

static double random_value(void) {
    return ((double)(next_random() % 2001) - 1000.0) / 1000.0;
}

static double get_time_seconds(void) {
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

static size_t get_peak_memory_bytes(void) {
    PROCESS_MEMORY_COUNTERS info;
    info.cb = sizeof(info);

    if (GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
        return (size_t)info.PeakWorkingSetSize;
    }

    return 0;
}

static double *allocate_dense(size_t n) {
    if (n == 0 || n > SIZE_MAX / n ||
        n * n > SIZE_MAX / sizeof(double)) {
        return NULL;
    }

    return (double *)calloc(n * n, sizeof(double));
}

static void free_csr(CSRMatrix *M) {
    free(M->values);
    free(M->col_indices);
    free(M->row_ptr);

    M->values = NULL;
    M->col_indices = NULL;
    M->row_ptr = NULL;
    M->nnz = 0;
}

static int dense_to_csr(const double *dense, size_t n,
                        double zero_probability, CSRMatrix *M) {
    size_t count = 0;

    M->rows = n;
    M->cols = n;
    M->nnz = 0;
    M->values = NULL;
    M->col_indices = NULL;
    M->row_ptr = NULL;

    M->row_ptr = (size_t *)malloc((n + 1) * sizeof(size_t));
    if (M->row_ptr == NULL) {
        return 0;
    }

    M->row_ptr[0] = 0;

    for (size_t i = 0; i < n * n; i++) {
        if (dense[i] != 0.0) {
            count++;
        }
    }

    M->values = (double *)malloc(count * sizeof(double));
    M->col_indices = (size_t *)malloc(count * sizeof(size_t));

    if (count > 0 && (M->values == NULL || M->col_indices == NULL)) {
        free_csr(M);
        return 0;
    }

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            double value = dense[i * n + j];

            if (value != 0.0) {
                M->values[M->nnz] = value;
                M->col_indices[M->nnz] = j;
                M->nnz++;
            }
        }

        M->row_ptr[i + 1] = M->nnz;
    }

    (void)zero_probability;
    return 1;
}

static size_t count_nonzero(const double *M, size_t n) {
    size_t count = 0;

    for (size_t i = 0; i < n * n; i++) {
        if (M[i] != 0.0) {
            count++;
        }
    }

    return count;
}

static void multiply_dense(const double *A, const double *B,
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

static void multiply_csr_dense(const CSRMatrix *A, const double *B,
                               double *C, size_t n) {
    for (size_t i = 0; i < A->rows; i++) {
        for (size_t p = A->row_ptr[i]; p < A->row_ptr[i + 1]; p++) {
            size_t k = A->col_indices[p];
            double a = A->values[p];

            for (size_t j = 0; j < n; j++) {
                C[i * n + j] += a * B[k * n + j];
            }
        }
    }
}

static int matrices_equal(const double *A, const double *B, size_t n) {
    for (size_t i = 0; i < n * n; i++) {
        double difference = fabs(A[i] - B[i]);
        double scale = fmax(1.0, fabs(A[i]));

        if (difference > 1e-8 * scale) {
            return 0;
        }
    }

    return 1;
}


static void save_result(size_t n, int zero_percent, size_t nnz,
                        size_t output_nnz,
                        double conversion_time, double dense_time,
                        double sparse_time, size_t dense_bytes,
                        size_t csr_bytes, size_t peak_bytes) {

    FILE *file = fopen("results/sparse_results.csv", "a+");

    if (file == NULL) {
        fprintf(stderr, "Could not open results/sparse_results.csv\n");
        return;
    }

    fseek(file, 0, SEEK_END);

    if (ftell(file) == 0) {
        fprintf(file,
                "size,zero_percent,nnz,density,output_nnz,"
                "output_density,conversion_seconds,dense_seconds,"
                "csr_seconds,dense_storage_bytes,csr_storage_bytes,"
                "peak_working_set_bytes,numeric_type,workers,"
                "warmup_repetitions,measured_repetitions\n");
    }

    double density = (double)nnz / ((double)n * (double)n);
    double output_density =
        (double)output_nnz / ((double)n * (double)n);

    fprintf(file,
            "%zu,%d,%zu,%.8f,%zu,%.8f,%.9f,%.9f,%.9f,"
            "%zu,%zu,%zu,double,1,1,%d\n",
            n, zero_percent, nnz, density,
            output_nnz, output_density,
            conversion_time, dense_time, sparse_time,
            dense_bytes, csr_bytes, peak_bytes, REPETITIONS);

    fclose(file);
}

int main(int argc, char *argv[]) {
    size_t n = 256;
    int zero_percent = 90;

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

    if (argc > 2) {
        char *end;
        long parsed = strtol(argv[2], &end, 10);

        if (*argv[2] == '\0' || *end != '\0' ||
            parsed < 0 || parsed > 100) {
            fprintf(stderr, "Zero percentage must be between 0 and 100.\n");
            return 1;
        }

        zero_percent = (int)parsed;
    }

    if (n > SIZE_MAX / n || n * n > SIZE_MAX / sizeof(double) ||
        n == SIZE_MAX) {
        fprintf(stderr, "Matrix size is too large.\n");
        return 1;
    }

    double *A = allocate_dense(n);
    double *B = allocate_dense(n);
    double *C_dense = allocate_dense(n);
    double *C_sparse = allocate_dense(n);

    if (A == NULL || B == NULL || C_dense == NULL || C_sparse == NULL) {
        fprintf(stderr, "Dense allocation failed for n=%zu.\n", n);
        free(A);
        free(B);
        free(C_dense);
        free(C_sparse);
        return 1;
    }

    rng_state = SEED;

    for (size_t i = 0; i < n * n; i++) {
    double candidate = random_value();
    uint64_t draw = next_random() % 100;

    if (candidate == 0.0) {
        candidate = 0.001;
    }

    A[i] = draw < (uint64_t)(100 - zero_percent)
               ? candidate
               : 0.0;

    B[i] = random_value();
}

    CSRMatrix csr = {0};

    double conversion_start = get_time_seconds();
    int converted = dense_to_csr(A, n, zero_percent / 100.0, &csr);
    double conversion_time = get_time_seconds() - conversion_start;

    if (!converted) {
        fprintf(stderr, "CSR conversion failed.\n");
        free(A);
        free(B);
        free(C_dense);
        free(C_sparse);
        free_csr(&csr);
        return 1;
    }

    /* Warm-up both multiplication methods. */
    multiply_dense(A, B, C_dense, n);
    multiply_csr_dense(&csr, B, C_sparse, n);

    if (!matrices_equal(C_dense, C_sparse, n)) {
        fprintf(stderr, "Validation FAILED.\n");
        free(A);
        free(B);
        free(C_dense);
        free(C_sparse);
        free_csr(&csr);
        return 1;
    }

    double dense_times[REPETITIONS];
    double sparse_times[REPETITIONS];

    for (int r = 0; r < REPETITIONS; r++) {
        memset(C_dense, 0, n * n * sizeof(double));
        double start = get_time_seconds();
        multiply_dense(A, B, C_dense, n);
        dense_times[r] = get_time_seconds() - start;

        memset(C_sparse, 0, n * n * sizeof(double));
        start = get_time_seconds();
        multiply_csr_dense(&csr, B, C_sparse, n);
        sparse_times[r] = get_time_seconds() - start;
    }

    /* Median for three repetitions. */
    for (int i = 0; i < REPETITIONS - 1; i++) {
        for (int j = i + 1; j < REPETITIONS; j++) {
            if (dense_times[j] < dense_times[i]) {
                double tmp = dense_times[i];
                dense_times[i] = dense_times[j];
                dense_times[j] = tmp;
            }

            if (sparse_times[j] < sparse_times[i]) {
                double tmp = sparse_times[i];
                sparse_times[i] = sparse_times[j];
                sparse_times[j] = tmp;
            }
        }
    }

    if (!matrices_equal(C_dense, C_sparse, n)) {
        fprintf(stderr, "Validation FAILED after timing.\n");
        free(A);
        free(B);
        free(C_dense);
        free(C_sparse);
        free_csr(&csr);
        return 1;
    }

    size_t dense_bytes = n * n * sizeof(double);
    size_t csr_bytes = csr.nnz * (sizeof(double) + sizeof(size_t))
                       + (n + 1) * sizeof(size_t);
    size_t output_nnz = count_nonzero(C_sparse, n);
    size_t peak_bytes = get_peak_memory_bytes();

    printf("Matrix size: %zu x %zu\n", n, n);
    printf("Zero percentage requested: %d%%\n", zero_percent);
    printf("Actual zero percentage: %.2f%%\n",
           100.0 * (1.0 - (double)csr.nnz / ((double)n * n)));
    printf("Non-zero entries: %zu\n", csr.nnz);
    printf("Validation OK\n");
    printf("Conversion time: %.6f seconds\n", conversion_time);
    printf("Dense median: %.6f seconds\n", dense_times[1]);
    printf("CSR median: %.6f seconds\n", sparse_times[1]);
    printf("Dense matrix storage: %.2f MiB\n",
           dense_bytes / (1024.0 * 1024.0));
    printf("CSR storage: %.2f MiB\n",
           csr_bytes / (1024.0 * 1024.0));
    printf("Peak working set: %.2f MiB\n",
           peak_bytes / (1024.0 * 1024.0));

    save_result(n, zero_percent, csr.nnz, output_nnz,
            conversion_time, dense_times[1], sparse_times[1],
            dense_bytes, csr_bytes, peak_bytes);

    free(A);
    free(B);
    free(C_dense);
    free(C_sparse);
    free_csr(&csr);

    return 0;
}
