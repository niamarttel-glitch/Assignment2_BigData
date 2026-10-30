# Assignment 2: Optimised Matrix Multiplication Approaches and Sparse Matrices

## 1. Objective

This report compares three sequential dense matrix multiplication algorithms and evaluates Compressed Sparse Row (CSR) storage for matrices containing many zeros. Execution time and storage are examined separately, including the cost of converting a dense matrix to CSR.

An additional memory experiment estimates the largest tested dense matrix size that fits within an explicitly defined experimental memory budget. Intel VTune Profiler is also used to investigate potential memory-related performance bottlenecks in the dense implementations.

## 2. Methodology

### 2.1 Dense multiplication

The dense implementation uses square `double` matrices and compares three algorithms:

- **`ijk`**: baseline triple-loop implementation.
- **`ikj`**: loop-reordered implementation.
- **`blocked`**: tiled implementation with a block size of 32.

The tested matrix sizes were 256, 512, 1024 and 2048. All three algorithms were measured at every size. All methods use sequential execution with one worker.

Input values are deterministic pseudo-random multiples of 0.001 in `[-1, 1]`, generated using seed `123456789`. The result is checked against the baseline implementation for correctness. Each method performs one warm-up followed by three measured repetitions. The reported execution time is the median of those three measurements.

The multiplication timer excludes warm-up and matrix-clearing work. The output matrix is reset before each measured repetition.

### 2.2 Sparse multiplication

The sparse implementation converts matrix A to **Compressed Sparse Row (CSR)** and multiplies it by dense matrix B, producing dense matrix C. The tested sizes were 256, 512 and 1024, with target zero percentages of 50%, 70%, 90% and 95%.

Here, **sparsity** means the percentage of zero entries, while **density** means the percentage of nonzero entries. The CSV reports the actual `nnz` (number of nonzero entries) and actual density, which can differ slightly from the target due to random generation.

Conversion time is recorded separately from dense and CSR multiplication times. The estimated end-to-end CSR time is:

`conversion_seconds + csr_seconds`

Each configuration performs one warm-up and three measured repetitions. The sparse CSV stores the median timing for each configuration.

The CSR result is validated against dense multiplication using the same logical A and B within the sparse program.

The dense and sparse programs use different pseudo-random generator implementations. Therefore, their generated matrices should not be described as identical across Parts A and B. The matched dense-versus-CSR comparison within each sparse experiment remains valid.

### 2.3 Memory measurements and limitations

Peak working set is a process-level operating-system measurement of resident memory. It includes allocations and runtime overhead and does not isolate the memory used by the multiplication algorithm alone. It is also not equivalent to the process's total committed memory.

The storage comparison in the sparse experiments concerns matrix A only. Matrices B and C remain dense in the CSR implementation.

Two distinct sets of dense measurements are reported:

1. **Original benchmarks**, used to compare execution times across algorithms and to measure the original process working set.
2. **Additional memory-budget experiments**, using a 64-bit executable, the `blocked` method and a budget of 256 MiB.

The additional experiments used three measured repetitions after one warm-up, and each tested configuration passed validation. Their execution times were obtained under different conditions from the original benchmarks and must not be substituted into the original timing table.

The 256 MiB budget is an experimental assumption, not a limit specified by the assignment. The largest feasible size identified in the additional experiments is therefore the largest confirmed size under this chosen measurement criterion, rather than a universal maximum for the computer.

## 3. Dense multiplication results

Median execution times are reported in seconds. Speedup is calculated as the baseline `ijk` time divided by the optimised method's time.

| Size | `ijk` (s) | `ikj` (s) | `blocked` (s) | `ijk` / `ikj` | `ijk` / `blocked` |
|---:|---:|---:|---:|---:|---:|
| 256 | 0.011277 | 0.007134 | 0.006841 | 1.58× | 1.65× |
| 512 | 0.398556 | 0.054901 | 0.053830 | 7.26× | 7.40× |
| 1024 | 4.037054 | 0.520242 | 0.449929 | 7.76× | 8.97× |
| 2048 | 111.137877 | 4.753414 | 3.778137 | 23.38× | 29.42× |

### 3.1 Discussion

Both optimisations substantially reduce runtime compared with `ijk` at every tested size.

At size 256, `ikj` and `blocked` have similar execution times, with `blocked` being slightly faster. At sizes 512, 1024 and 2048, `blocked` also obtains the lowest median execution time.

The difference becomes more noticeable as matrix size increases. At size 2048, `blocked` is approximately 29.42 times faster than the baseline `ijk` implementation.

These findings are consistent with improvements in memory locality from loop reordering and tiling, although execution-time measurements alone do not directly establish the number of cache misses or prove a specific hardware bottleneck.

## 4. Sparse CSR results

Times are reported in seconds. **CSR total** includes conversion plus CSR multiplication. The first speedup column compares dense multiplication against CSR multiplication alone. The second compares dense multiplication against the estimated total CSR cost, including conversion.

| Size | Zeros (%) | Actual density (%) | A dense (bytes) | A CSR (bytes) | Dense multiply (s) | CSR multiply (s) | Conversion (s) | CSR total (s) | Dense / CSR | Dense / total |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 256 | 50 | 50.02 | 524,288 | 394,364 | 0.007598 | 0.003529 | 0.000811 | 0.004340 | 2.15× | 1.75× |
| 256 | 70 | 29.96 | 524,288 | 236,624 | 0.007611 | 0.001998 | 0.000652 | 0.002650 | 3.81× | 2.87× |
| 256 | 90 | 10.01 | 524,288 | 79,748 | 0.007248 | 0.000717 | 0.000339 | 0.001056 | 10.11× | 6.86× |
| 256 | 95 | 4.90 | 524,288 | 39,596 | 0.006790 | 0.000350 | 0.000195 | 0.000545 | 19.40× | 12.46× |
| 512 | 50 | 49.91 | 2,097,152 | 1,571,952 | 0.051723 | 0.026313 | 0.003006 | 0.029319 | 1.97× | 1.76× |
| 512 | 70 | 29.85 | 2,097,152 | 941,100 | 0.056267 | 0.017572 | 0.002575 | 0.020147 | 3.20× | 2.79× |
| 512 | 90 | 9.98 | 2,097,152 | 315,864 | 0.052704 | 0.005715 | 0.001172 | 0.006887 | 9.22× | 7.65× |
| 512 | 95 | 4.99 | 2,097,152 | 159,024 | 0.057411 | 0.002800 | 0.001096 | 0.003896 | 20.50× | 14.74× |
| 1024 | 50 | 49.98 | 8,388,608 | 6,292,832 | 0.500754 | 0.241228 | 0.013385 | 0.254613 | 2.08× | 1.97× |
| 1024 | 70 | 29.95 | 8,388,608 | 3,772,292 | 0.518691 | 0.149686 | 0.009457 | 0.159143 | 3.47× | 3.26× |
| 1024 | 90 | 9.99 | 8,388,608 | 1,260,884 | 0.522450 | 0.049501 | 0.004853 | 0.054354 | 10.55× | 9.61× |
| 1024 | 95 | 4.98 | 8,388,608 | 631,232 | 0.522942 | 0.025395 | 0.003327 | 0.028722 | 20.59× | 18.21× |

### 4.1 Storage and output density

CSR storage for A decreases as the number of zeros increases. At size 1024 and approximately 95% zeros, A uses 631,232 bytes in CSR compared with 8,388,608 bytes in dense storage, approximately 7.5% of the dense storage requirement for A.

At approximately 50% zeros, CSR uses 6,292,832 bytes, around 75% of dense A storage, so the storage advantage is smaller.

All tested products have `output_nnz = n × n` and output density 1.0 (100%). Thus, C is fully dense in these experiments. Sparse input does not guarantee sparse output, and this implementation continues to allocate dense storage for B and C.

### 4.2 Timing and conversion cost

CSR multiplication is faster in every tested configuration, and the advantage generally increases with sparsity.

For size 1024 with approximately 95% zeros, CSR multiplication takes 0.025395 seconds compared with 0.522942 seconds for dense multiplication, approximately 20.59× faster. Including conversion gives an estimated CSR total of 0.028722 seconds, which remains approximately 18.21× faster than dense multiplication.

At size 1024 with approximately 50% zeros, CSR is approximately 2.08× faster for multiplication alone and 1.97× faster including conversion.

CSR indexing and row-pointer traversal add overhead, so the advantage is smaller when many entries are nonzero. If the same CSR matrix is reused for multiple multiplications, conversion can be amortised. For a one-off multiplication, conversion should be included in the end-to-end comparison.

Because C is dense, these results demonstrate the benefits of sparse input rather than a fully sparse matrix-matrix product with sparse output storage.

## 5. Memory use and observed bottlenecks

Peak working set is a process-level measurement and includes allocations and runtime overhead. It should not be interpreted as the exact memory consumed exclusively by the matrix data structures.

For a fixed size, sparse peak working set generally decreases as A becomes more sparse, but not in direct proportion to CSR storage because B and C remain dense and other process allocations are included.

Potential bottlenecks include dense matrix memory access, CSR index traversal, dense-to-CSR conversion, and allocation of the dense output. The additional VTune analysis investigates memory-related behaviour in the dense algorithms, while the memory-budget experiments estimate the largest tested matrix size that fits within the chosen working-set budget.

### 5.1 Cache miss analysis with Intel VTune Profiler

Intel VTune Profiler was used to analyse the `blocked` and `ikj` dense matrix multiplication algorithms with matrices of size 2048 × 2048. Both analyses used the same 64-bit executable generated from `dense.c`.

The thread-level analysis of the `ikj` profile showed that the main thread accounted for 185.760 seconds of CPU time and all reported load, store and LLC miss events. A second thread was detected, but VTune attributed no CPU time or memory events to it. Therefore, the higher thread count does not provide evidence of parallel execution in `ikj`. The origin of the additional thread could not be determined from the available profiling data, and the difference in thread counts should be considered when interpreting the comparison.

| Metric | `blocked` | `ikj` |
|---|---:|---:|
| Median execution time | 16.806 s | 17.024 s |
| LLC misses | 7,883,609,336 | 7,994,806,019 |
| P-core Memory Bound | 49.0% | 50.5% |
| P-core DRAM Bound | 52.5% | 53.8% |
| Average DRAM bandwidth | 4.298 GB/s | 5.858 GB/s |
| Peak working set | 131.02 MiB | 131.02 MiB |

The `blocked` implementation recorded approximately 111 million fewer last-level cache (LLC) misses than `ikj`, a reduction of approximately 1.39%. Its median execution time was also approximately 0.22 seconds lower, making it around 1.28% faster in these profiling runs.

Both implementations showed substantial memory-bound behaviour on the performance cores, with Memory Bound values of 49.0% for `blocked` and 50.5% for `ikj`. The corresponding DRAM Bound values were 52.5% and 53.8%. These measurements suggest that memory-related stalls may contribute to execution time, although they do not establish that memory access is the only performance bottleneck.

Overall, `blocked` showed a modest advantage in this profiling run, with fewer LLC misses and a slightly shorter median execution time. However, VTune reported different total thread counts for the two runs (one for `blocked` and two for `ikj`), so the results should be interpreted cautiously rather than as a perfectly controlled comparison.

These profiled execution times are specific to the VTune runs and should not replace the execution times reported in the original benchmark table, which were obtained under different conditions.

### 5.2 Maximum tested matrix size under a memory budget

Additional experiments were performed to estimate the largest feasible dense matrix size under an experimental budget of 256 MiB of peak working set. The same 64-bit executable and the `blocked` method were used for each configuration. Each test included one warm-up and three measured repetitions. All tested configurations completed successfully and passed validation.

| Matrix size | Median time (s) | Peak working set (MiB) | Within 256 MiB |
|---:|---:|---:|:---:|
| 2560 × 2560 | 32.254669 | 203.05 | Yes |
| 2816 × 2816 | 42.688805 | 245.06 | Yes |
| 2848 × 2848 | 43.555928 | 250.55 | Yes |
| 2864 × 2864 | 51.131307 | 253.37 | Yes |
| 2880 × 2880 | 44.992380 | 256.14 | No |
| 3072 × 3072 | 55.672106 | 291.04 | No |

The largest tested configuration that remained within the budget was **2864 × 2864**, with a peak working set of 253.37 MiB. This leaves approximately 2.63 MiB below the selected budget. The 2880 × 2880 configuration slightly exceeded it, reaching 256.14 MiB, while the 3072 × 3072 configuration reached 291.04 MiB.

These experiments establish 2864 × 2864 as the largest feasible size confirmed by the tests under the selected criterion. They do not prove that it is the absolute maximum possible size, since not every intermediate size was tested and working-set measurements can vary between executions. The 256 MiB budget is an experimental assumption rather than a hardware limit specified by the assignment. Furthermore, peak working set measures resident process memory and does not capture all committed memory.

## 6. Reproducibility

The main project files are:

- `src/c/dense.c`: dense `ijk`, `ikj` and blocked multiplication.
- `src/c/sparse.c`: CSR conversion and sparse-by-dense multiplication.
- `scripts/run_benchmarks.py`: runs the benchmark configurations.
- `scripts/plot_results.py`: generates plots from result CSVs.
- `scripts/generate_data.py`: documents experiment configuration and writes `results/generation_config.json`; matrices are generated inside the C programs.

Compile from the project root using GCC on Windows:

```powershell
gcc -std=c11 -O2 -Wall -Wextra src/c/dense.c -o src/c/dense.exe -lm -lpsapi
gcc -std=c11 -O2 -Wall -Wextra src/c/sparse.c -o src/c/sparse.exe -lm -lpsapi
```

Run the complete benchmark suite:

```powershell
python .\scripts\run_benchmarks.py
```

Generate the plots from the existing CSV files:

```powershell
python .\scripts\plot_results.py
```

Generate the experimental configuration file:

```powershell
python .\scripts\generate_data.py
```

Individual C experiments can also be run manually:

```powershell
.\src\c\dense.exe 1024 all
.\src\c\sparse.exe 1024 95
```

**Important:** the C programs append measurements to the result CSVs. The benchmark runner skips configurations already present in the CSV files, but manually executing an experiment that has already been recorded can add duplicate rows. Back up or rename existing CSVs before rerunning individual experiments if a fresh dataset is required.

The additional memory-budget tests used the existing 64-bit executable and were run separately from the original benchmark data. Their results should be kept separate from the original CSVs and timing comparisons.

## 7. Conclusion

The dense multiplication results show that loop reordering and blocking substantially improve performance over the baseline at all tested sizes. The benefit increases with matrix size. `blocked` obtains the lowest median execution time in the final benchmark dataset, reaching a speedup of approximately 29.42× over `ijk` at size 2048.

CSR is particularly beneficial when matrix A is highly sparse. Its storage requirement and multiplication time decrease as the proportion of zeros rises. Conversion does not erase the advantage in the tested configurations, but it must be included in end-to-end comparisons. At size 1024 and approximately 95% zeros, the estimated total CSR time, including conversion, is approximately 18.21 times faster than dense multiplication.

The results apply to the tested implementation and configurations; sparsity percentage alone does not universally determine performance because indexing overhead, matrix structure, conversion cost, dense operands and output density also matter.

The VTune profiles show that both `blocked` and `ikj` exhibit substantial memory-bound behaviour on the performance cores. In the profiling runs, `blocked` recorded fewer LLC misses and a slightly lower median execution time. However, the difference was modest, and the differing reported thread counts mean that the comparison should be interpreted cautiously.

Finally, the additional memory experiments identified **2864 × 2864** as the largest tested dense matrix that remained within the chosen 256 MiB peak-working-set budget. The next tested size, 2880 × 2880, exceeded the budget slightly. This provides a practical estimate of feasible matrix size under the experimental criterion, but not proof of an absolute maximum.

Overall, the experiments demonstrate the trade-offs between algorithmic optimisation, memory behaviour, sparse storage and conversion overhead.