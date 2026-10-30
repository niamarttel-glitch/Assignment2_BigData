import csv
from pathlib import Path

files = [
    Path("results/dense_results.csv"),
    Path("results/sparse_results.csv"),
]

metadata = {
    "numeric_type": "double",
    "workers": 1,
    "warmup_repetitions": 1,
    "measured_repetitions": 3,
}

for file in files:
    with file.open("r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        rows = list(reader)
        fieldnames = reader.fieldnames + list(metadata.keys())

    for row in rows:
        row.update(metadata)

    with file.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Actualizado: {file}")