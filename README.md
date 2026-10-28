# Optimised Matrix Multiplication Approaches and Sparse Matrices

## Overview

This project investigates ways to improve matrix multiplication
performance and reduce memory usage when matrices contain many zero
values. It compares three dense matrix multiplication loop orders and
evaluates a Compressed Sparse Row (CSR) representation for sparse
matrices.

The implementation is written in C, with Python scripts for running
experiments, recording configuration information, and plotting results.

## Project structure

``` text
.
├── src/
│   └── c/
│       ├── dense.c
│       ├── dense.exe
│       ├── sparse.c
│       └── sparse.exe
├── scripts/
│   ├── generate_data.py
│   ├── plot_results.py
│   └── run_benchmarks.py
├── results/
│   ├── dense_results.csv
│   ├── sparse_results.csv
│   ├── generation_config.json
│   └── *.png
├── report.md
└── README.md
```

The `.exe` files are compiled Windows executables. They can be
regenerated from the C source files using the commands below.

## Requirements

-   Windows with PowerShell, or another environment with a compatible C
    compiler.
-   GCC (for example, MinGW-w64 GCC).
-   Python 3.
-   The Python packages required by `scripts/plot_results.py`, if any.
    Install them according to that script's imports.

## Compilation

Run these commands from the project root directory:

``` powershell
gcc -std=c11 -O2 -Wall -Wextra src/c/dense.c -o src/c/dense.exe -lm -lpsapi
gcc -std=c11 -O2 -Wall -Wextra src/c/sparse.c -o src/c/sparse.exe -lm -lpsapi
```

`-O2` enables compiler optimisations. `-Wall -Wextra` enable additional
compiler warnings. `-lm` links the math library; `-lpsapi` is used for
process memory measurements on Windows.

## Running the programs

Run commands from the project root.

### Dense matrix multiplication

``` powershell
.\src\c\dense.exe 256 all
.\src\c\dense.exe 1024 ijk
.\src\c\dense.exe 1024 ikj
.\src\c\dense.exe 1024 blocked
```

The command-line arguments are:

``` text
dense.exe [matrix_size] [all|ijk|ikj|blocked]
```

-   `ijk`: baseline loop order.
-   `ikj`: changes the loop order to improve access to contiguous
    elements.
-   `blocked`: processes matrix tiles to improve cache locality. The
    current block size is 32.

### Sparse matrix multiplication

``` powershell
.\src\c\sparse.exe 256 90
.\src\c\sparse.exe 1024 95
```

The command-line arguments are:

``` text
sparse.exe [matrix_size] [zero_percentage]
```

The sparse program compares multiplication using a dense representation
of matrix A with multiplication using A stored in CSR format. The
right-hand matrix B and output matrix C remain dense.

## Running the experiment suite

The benchmark script runs `ikj` and `blocked` for dense matrices of
sizes 256, 512, 1024, and 2048, and CSR experiments for sizes 256, 512,
and 1024 at 50%, 70%, 90%, and 95% target zeros.

``` powershell
python scripts/run_benchmarks.py
```

**Important:** the script writes benchmark output to the CSV files.
Running it again may append duplicate rows to existing results,
depending on the C programs' output-writing behaviour. Back up the
current CSV files before rerunning the full suite, and inspect the CSV
headers and rows afterwards. The baseline `ijk` results currently
recorded for sizes 256, 512, and 1024 are already present in
`results/dense_results.csv`; the suite does not run `ijk` as part of its
dense loop.

The `generate_data.py` script records or documents the experiment
configuration; the C programs generate the matrix values used in each
experiment. See the script for its exact behaviour.

## Results and interpretation

See [`report.md`](report.md) for the experimental tables and discussion.

The recorded dense results show that `ikj` and `blocked` are
substantially faster than the baseline `ijk` for the tested sizes. At
size 1024, the median time is approximately 4.56 s for `ijk`, 0.654 s
for `ikj`, and 0.543 s for `blocked`. At size 2048, the recorded medians
are approximately 5.83 s for `ikj` and 4.46 s for `blocked`; no `ijk`
measurement is currently recorded for that size.

The sparse results show that CSR multiplication becomes more
advantageous as the number of zeros increases. For size 1024 with a
target of 95% zeros, the recorded CSR multiplication time is
approximately 0.0291 s, compared with 0.611 s for dense multiplication.
CSR conversion took approximately 0.00305 s, so conversion plus CSR
multiplication took approximately 0.0322 s for that experiment.

These are measurements from the current test environment, not universal
performance guarantees. Actual density may differ slightly from the
target zero percentage. The product matrix C was fully dense in the
recorded experiments, so CSR was used only for input matrix A.

## Reproducibility and measurement notes

-   Dense multiplication timings use three measured repetitions after a
    warm-up; reported comparisons in the report use medians.
-   The sparse implementation measures conversion time separately from
    multiplication time.
-   The reported CSR storage size refers to matrix A's CSR arrays; dense
    storage refers to matrix A as a dense array. Matrix B and result
    matrix C are dense in both approaches.
-   Peak working set is a process-level memory metric and should not be
    interpreted as memory used exclusively by the multiplication
    algorithm.
-   The dense and sparse programs use separate random-number-generation
    implementations. Do not assume that their generated matrices are
    identical across Part A and Part B.
-   The exact commands and detailed results are documented in
    `report.md`.

## Notes

This README describes the project files and experiment results currently
recorded. If the source code, CSV output format, or experiment
configuration changes, update this document and `report.md` accordingly.
