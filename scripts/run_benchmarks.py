
import csv
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DENSE_EXE = ROOT / "src" / "c" / "dense.exe"
SPARSE_EXE = ROOT / "src" / "c" / "sparse.exe"
RESULTS = ROOT / "results"

DENSE_SIZES = [256, 512, 1024, 2048]
SPARSE_SIZES = [256, 512, 1024]
ZERO_PERCENTAGES = [50, 70, 90, 95]


def run_program(executable, *args):
    result = subprocess.run(
        [str(executable), *map(str, args)],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=180,
    )

    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr)
        raise RuntimeError(
            f"Falló {executable.name} con argumentos {args}"
        )

    print(result.stdout)


def main():
    RESULTS.mkdir(exist_ok=True)

    if not DENSE_EXE.exists() or not SPARSE_EXE.exists():
        raise FileNotFoundError(
            "Faltan los ejecutables. Compila dense.c y sparse.c primero."
        )

    print("=== EXPERIMENTOS DENSOS ===")

    for n in DENSE_SIZES:
        for method in ["ikj", "blocked"]:
            print(f"\nDenso: n={n}, método={method}")
            run_program(DENSE_EXE, n, method)

    print("\n=== EXPERIMENTOS DISPERSOS ===")

    for n in SPARSE_SIZES:
        for zeros in ZERO_PERCENTAGES:
            print(f"\nCSR: n={n}, ceros={zeros}%")
            run_program(SPARSE_EXE, n, zeros)

    print("\nExperimentos completados.")
    print("Revisa los CSV de la carpeta results.")


if __name__ == "__main__":
    main()
