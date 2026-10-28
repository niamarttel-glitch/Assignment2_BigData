
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

results = Path("results")
results.mkdir(exist_ok=True)

# 1. Dense matrix multiplication
dense = pd.read_csv(results / "dense_results.csv")

summary = (
    dense.groupby(["size", "method"])["time_seconds"]
    .median()
    .reset_index()
)

plt.figure()
for method, group in summary.groupby("method"):
    plt.plot(group["size"], group["time_seconds"], marker="o", label=method)

plt.xlabel("Matrix size (n)")
plt.ylabel("Median execution time (seconds)")
plt.title("Dense matrix multiplication performance")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(results / "dense_times.png", dpi=150)
plt.close()

# 2. Sparse matrix multiplication
sparse = pd.read_csv(results / "sparse_results.csv")

plt.figure()
for zero_percent, group in sparse.groupby("zero_percent"):
    plt.plot(
        group["size"],
        group["csr_seconds"],
        marker="o",
        label=f"{zero_percent}% zeros"
    )

plt.xlabel("Matrix size (n)")
plt.ylabel("CSR execution time (seconds)")
plt.title("Sparse matrix multiplication performance")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(results / "sparse_times.png", dpi=150)
plt.close()

# 3. Dense vs CSR execution time

# 3. Dense vs CSR execution time at n=1024
comparison = sparse[sparse["size"] == 1024].sort_values("zero_percent")

plt.figure(figsize=(8, 5))

plt.plot(
    comparison["zero_percent"],
    comparison["dense_seconds"],
    marker="o",
    linewidth=2,
    label="Dense"
)

plt.plot(
    comparison["zero_percent"],
    comparison["csr_seconds"],
    marker="o",
    linewidth=2,
    label="CSR"
)

plt.xlabel("Percentage of zeros (%)")
plt.ylabel("Median execution time (seconds)")
plt.title("Dense vs CSR multiplication (n=1024)")
plt.xticks([50, 70, 90, 95])
plt.legend()
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(results / "dense_vs_csr.png", dpi=150)
plt.close()

print("Plots saved in results/:")
print("- dense_times.png")
print("- sparse_times.png")
print("- dense_vs_csr.png")
