
import csv
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

DENSE_EXE = ROOT / "src" / "c" / "dense.exe"
SPARSE_EXE = ROOT / "src" / "c" / "sparse.exe"

RESULTS = ROOT / "results"

DENSE_CSV = RESULTS / "dense_results.csv"
SPARSE_CSV = RESULTS / "sparse_results.csv"

DENSE_SIZES = [256, 512, 1024, 2048]
DENSE_METHODS = ["ijk", "ikj", "blocked"]

SPARSE_SIZES = [256, 512, 1024]
ZERO_PERCENTAGES = [50, 70, 90, 95]

TIMEOUT = 600


def run_program(executable, *args):
    print(f"Ejecutando: {executable.name} {' '.join(map(str, args))}")

    result = subprocess.run(
        [str(executable), *map(str, args)],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=TIMEOUT,
    )

    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr)

        raise RuntimeError(
            f"Falló {executable.name} con argumentos {args}"
        )

    print(result.stdout)


def read_dense_results():
    completed = set()

    if not DENSE_CSV.exists():
        return completed

    with DENSE_CSV.open(
        "r",
        newline="",
        encoding="utf-8"
    ) as file:

        reader = csv.DictReader(file)

        for row in reader:
            key = (
                int(row["size"]),
                row["method"]
            )

            completed.add(key)

    return completed


def read_sparse_results():
    completed = set()

    if not SPARSE_CSV.exists():
        return completed

    with SPARSE_CSV.open(
        "r",
        newline="",
        encoding="utf-8"
    ) as file:

        reader = csv.DictReader(file)

        for row in reader:
            key = (
                int(row["size"]),
                int(row["zero_percent"])
            )

            completed.add(key)

    return completed


def main():
    RESULTS.mkdir(exist_ok=True)

    if not DENSE_EXE.exists():
        raise FileNotFoundError(
            f"No se encuentra {DENSE_EXE}"
        )

    if not SPARSE_EXE.exists():
        raise FileNotFoundError(
            f"No se encuentra {SPARSE_EXE}"
        )

    dense_completed = read_dense_results()
    sparse_completed = read_sparse_results()

    print("=== EXPERIMENTOS DENSOS ===")

    for n in DENSE_SIZES:

        for method in DENSE_METHODS:

            if (n, method) in dense_completed:
                print(
                    f"Omitido: n={n}, método={method} "
                    "(ya tiene resultados)"
                )
                continue

            print(
                f"\nDenso: n={n}, método={method}"
            )

            run_program(
                DENSE_EXE,
                n,
                method
            )

    print("\n=== EXPERIMENTOS DISPERSOS ===")

    for n in SPARSE_SIZES:

        for zeros in ZERO_PERCENTAGES:

            if (n, zeros) in sparse_completed:
                print(
                    f"Omitido: n={n}, ceros={zeros}% "
                    "(ya tiene resultados)"
                )
                continue

            print(
                f"\nCSR: n={n}, ceros={zeros}%"
            )

            run_program(
                SPARSE_EXE,
                n,
                zeros
            )

    print("\nExperimentos completados.")
    print("Revisa los CSV de la carpeta results.")


if __name__ == "__main__":
    main()

