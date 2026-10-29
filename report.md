# Assignment 2: Optimised Matrix Multiplication Approaches and Sparse Matrices

## 1. Objective

This report compares three sequential dense matrix multiplication
algorithms and evaluates Compressed Sparse Row (CSR) storage for
matrices containing many zeros. Execution time and storage are examined
separately, including the cost of converting a dense matrix to CSR.

## 2. Methodology

### 2.1 Dense multiplication

The dense implementation uses square `double` matrices and compares: -
**`ijk`**: baseline triple-loop implementation. - **`ikj`**:
loop-reordered implementation. - **`blocked`**: tiled implementation
with block size 32.

Sizes were 256, 512, 1024 and 2048. The baseline `ijk` was measured at
sizes 256, 512 and 1024 only; it was not run at 2048. All methods are
sequential. Input values are deterministic pseudo-random multiples of
0.001 in \[-1, 1\], generated with seed `123456789`. The result is
checked against the baseline for correctness. A warm-up is performed,
followed by three measured repetitions; the table reports median times.
The multiplication timer excludes warm-up and matrix-clearing work.

### 2.2 Sparse multiplication

The sparse implementation converts matrix A to **Compressed Sparse Row
(CSR)** and multiplies it by dense matrix B, producing dense matrix C.
Sizes were 256, 512 and 1024, with target zero percentages of 50%, 70%,
90% and 95%.

Here, **sparsity** means the percentage of zero entries; **density**
means the percentage of nonzero entries. The CSV reports actual `nnz`
(number of nonzeros) and actual density, which can differ slightly from
the target due to random generation. The conversion time is recorded
separately from dense and CSR multiplication times. The estimated CSR
end-to-end time is `conversion_seconds + csr_seconds`. Each
configuration has a warm-up and three measured repetitions; the CSV
stores the timing summary. The CSR result is validated against dense
multiplication using the same logical A and B within the sparse program.

The dense and sparse programs use different pseudo-random generator
implementations, so their generated matrices should not be described as
identical across Parts A and B. The matched dense-versus-CSR comparison
within each sparse experiment is valid.

### 2.3 Measurement limitations

Peak working set is a process-level memory measurement, not the memory
used by the multiplication algorithm alone. The storage comparison below
concerns matrix A only; B and output C remain dense in the CSR
implementation. The largest dense size tested was 2048 and the largest
sparse size tested was 1024. These are the largest sizes tested, not a
verified maximum feasible size for the computer; no explicit memory
budget was defined.

The largest dense matrix size tested was 2048 × 2048, while the largest sparse matrix size tested was 1024 × 1024. The dense benchmark at size 2048 took approximately 15 minutes to execute, so the experiment was stopped to keep the overall evaluation time manageable. Consequently, these sizes represent the limits of the experiments performed, not a formally verified maximum hardware capacity. Larger matrices were not systematically tested.

## 3. Dense multiplication results

Median execution times are in seconds. Speedup is the baseline `ijk`
time divided by the optimised method time.

  -----------------------------------------------------------------------
         Size   `ijk` (s)   `ikj` (s)   `blocked`     `ijk` /     `ijk` /
                                              (s)       `ikj`   `blocked`
  ----------- ----------- ----------- ----------- ----------- -----------
          256    0.036382    0.009060    0.008841       4.02×       4.11×

          512    0.325446    0.070255    0.071121       4.63×       4.58×

         1024    4.559563    0.653556    0.542615       6.98×       8.40×

         2048         Not    5.829692    4.455106         ---         ---
                 measured                                     
  -----------------------------------------------------------------------

### Discussion

Both optimisations substantially reduce runtime compared with `ijk` at
sizes where all three methods were measured. At size 256, `ikj` and
`blocked` are close. At size 512, `ikj` is slightly faster than
`blocked`, so blocking does not guarantee an improvement at every size.
At sizes 1024 and 2048, `blocked` is faster than `ikj`. These results
are consistent with improved memory locality from loop reordering and
tiling, although cache misses were not measured directly. Since `ijk`
was not run at 2048, a baseline speedup cannot be reported for that
size.

## 4. Sparse CSR results

Times are in seconds. "CSR total" includes conversion plus CSR
multiplication. The speedup column compares dense multiplication against
CSR multiplication alone.

  --------------------------------------------------------------------------------------------------------
     Size   Zeros    Actual     A dense       A CSR      Dense        CSR   Conversion  CSR total  Dense /
              (%)   density     storage     storage   multiply   multiply          (s)        (s)      CSR
                        (%)     (bytes)     (bytes)        (s)        (s)                         
  ------- ------- --------- ----------- ----------- ---------- ---------- ------------ ---------- --------
      256      50     50.02     524,288     394,364   0.008634   0.004104     0.000950   0.005054    2.10×

      256      70     29.96     524,288     236,624   0.008311   0.002476     0.000602   0.003078    3.36×

      256      90     10.01     524,288      79,748   0.008783   0.000857     0.000298   0.001154   10.26×

      256      95      4.90     524,288      39,596   0.008560   0.000512     0.000209   0.000721   16.73×

      512      50     49.91   2,097,152   1,571,952   0.067882   0.034313     0.003412   0.037725    1.98×

      512      70     29.85   2,097,152     941,100   0.065371   0.020206     0.002555   0.022761    3.23×

      512      90      9.98   2,097,152     315,864   0.067319   0.007125     0.001291   0.008416    9.45×

      512      95      4.99   2,097,152     159,024   0.067808   0.003628     0.000914   0.004543   18.69×

     1024      50     49.98   8,388,608   6,292,832   0.651252   0.332062     0.012362   0.344424    1.96×

     1024      70     29.95   8,388,608   3,772,292   0.647719   0.182837     0.009167   0.192004    3.54×

     1024      90      9.99   8,388,608   1,260,884   0.605667   0.059042     0.005183   0.064225   10.26×

     1024      95      4.98   8,388,608     631,232   0.610994   0.029122     0.003053   0.032176   20.98×
  --------------------------------------------------------------------------------------------------------

### 4.1 Storage and output density

CSR storage for A falls as the number of zeros increases. At size 1024
and approximately 95% zeros, A uses 631,232 bytes in CSR compared with
8,388,608 bytes in dense storage, about 7.5% of dense A storage. At
approximately 50% zeros, CSR uses 6,292,832 bytes, around 75% of dense A
storage, so the storage advantage is smaller.

All tested products have `output_nnz = n × n` and output density 1.0
(100%). Thus C is fully dense in these experiments. Sparse input does
not guarantee sparse output, and this implementation still allocates
dense storage for B and C.

### 4.2 Timing and conversion cost

CSR multiplication is faster in every tested configuration, and the
advantage generally increases with sparsity. For size 1024 with
approximately 95% zeros, CSR multiplication takes 0.029122 seconds
compared with 0.610994 seconds for dense multiplication, about 21.0×
faster. Including conversion gives an estimated CSR total of 0.032176
seconds, still about 19.0× faster than dense multiplication.

At size 1024 with approximately 50% zeros, CSR is about 2.0× faster for
multiplication alone and 1.9× faster including conversion. CSR indexing
and row-pointer traversal add overhead, so the advantage is smaller when
many entries are nonzero. If the same CSR matrix is reused for multiple
multiplications, conversion can be amortised; for a one-off
multiplication, conversion should be included in end-to-end time.
Because C is dense, these results demonstrate sparse-input benefits
rather than a fully sparse matrix-matrix product with sparse output
storage.

## 5. Memory observations and bottlenecks

Dense peak working set rose from 6,565,888 bytes at size 256 to 138,641,408 bytes at size 2048. This is process-level memory, including allocations and runtime overhead, rather than an isolated measurement of algorithmic memory.

For a fixed size, sparse peak working set generally decreases as A becomes more sparse, but not in direct proportion to CSR storage because B and C remain dense and other process allocations are included. Likely bottlenecks include dense matrix memory access, CSR index traversal, dense-to-CSR conversion, and allocation of the dense output.

### 5.1. Cache Miss Analysis with Intel VTune Profiler

To investigate memory performance and potential bottlenecks, Intel VTune Profiler was used to analyse the `blocked` and `ikj` dense matrix multiplication algorithms with matrices of size 2048 × 2048. Both analyses used the same 64-bit executable generated from `dense.c`.

The thread-level analysis of the `ikj` profile showed that the main thread accounted for 185.760 seconds of CPU time and all reported load, store, and LLC miss events. A second thread was detected, but VTune attributed no CPU time or memory events to it. Therefore, the higher thread count does not provide evidence of parallel execution in `ikj`. However, the origin of the additional thread could not be determined from the available profiling data, and the difference in thread counts should be considered when interpreting the comparison.

| Metric | Blocked | ikj |
|---|---:|---:|
| Median execution time | 16.806 s | 17.024 s |
| LLC misses | 7,883,609,336 | 7,994,806,019 |
| P-core Memory Bound | 49.0% | 50.5% |
| P-core DRAM Bound | 52.5% | 53.8% |
| Average DRAM bandwidth | 4.298 GB/s | 5.858 GB/s |
| Peak working set | 131.02 MiB | 131.02 MiB |

The `blocked` implementation recorded approximately 111 million fewer last-level cache (LLC) misses than `ikj`, representing a reduction of approximately 1.39%. Its median execution time was also approximately 0.22 seconds lower, making it around 1.28% faster in these profiling runs.

Both implementations showed substantial memory-bound behaviour on the performance cores, with Memory Bound values of 49.0% for `blocked` and 50.5% for `ikj`. The corresponding DRAM Bound values were 52.5% and 53.8%. These measurements suggest that memory-related stalls may contribute to execution time, although they do not establish that memory access is the only performance bottleneck.

Overall, `blocked` showed a modest advantage in this profiling run, with fewer LLC misses and a slightly shorter median execution time. However, VTune reported different total thread counts for the two runs (one for `blocked` and two for `ikj`), so the results should be interpreted cautiously rather than as a perfectly controlled comparison.

These profiled execution times are specific to the VTune runs and should not replace the execution times reported in the main benchmark results, which were obtained under different conditions.

## 6. Reproducibility

Project files: - `src/c/dense.c`: dense `ijk`, `ikj` and blocked
multiplication. - `src/c/sparse.c`: CSR conversion and sparse-by-dense
multiplication. - `scripts/run_benchmarks.py`: runs the benchmark
configurations. - `scripts/plot_results.py`: generates plots from result
CSVs. - `scripts/generate_data.py`: documents experiment configuration
and writes `results/generation_config.json`; matrices are generated
inside the C programs.

Compile from the project root using GCC on Windows:

``` powershell
gcc -std=c11 -O2 -Wall -Wextra src/c/dense.c -o src/c/dense.exe -lm -lpsapi
gcc -std=c11 -O2 -Wall -Wextra src/c/sparse.c -o src/c/sparse.exe -lm -lpsapi
```

Run a dense experiment:

``` powershell
.\src\c\dense.exe 1024 all
```

Run a sparse experiment:

``` powershell
.\src\c\sparse.exe 1024 95
```

**Important:** the benchmark driver appends measurements to the result
CSVs through the C programs. Back up or rename existing CSVs before
rerunning the suite if a fresh dataset is required. Do not append a new
run and then treat the combined data as the original experiment.

## 7. Conclusion

The dense results show that loop reordering and blocking improve
performance over the baseline at tested sizes. Blocking is not always
faster for smaller matrices, but it is the fastest tested dense method
at sizes 1024 and 2048.

CSR is most beneficial when A is highly sparse: its storage requirement
and multiplication time decrease as the number of zeros rises.
Conversion does not erase the advantage in these experiments, but it
must be included in end-to-end comparisons. These conclusions apply to
this implementation and these test sizes; sparsity percentage alone does
not universally determine performance because indexing overhead, matrix
structure, conversion cost, dense operands and output density also
matter.
