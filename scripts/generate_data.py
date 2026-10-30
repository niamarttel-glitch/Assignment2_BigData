
"""
Input generation documentation and configuration for Assignment 2.

The C programs currently generate their matrices internally.
This script records the experiment configuration and reports
the input sizes and sparsity levels used by the benchmarks.
"""

import argparse
import json
from pathlib import Path

SEED = 123456789

DENSE_SIZES = [256, 512, 1024, 2048]
SPARSE_SIZES = [256, 512, 1024]
ZERO_PERCENTAGES = [50, 70, 90, 95]


def build_config():
    return {
        "seed": SEED,
        "numeric_type": "double",

        "execution": {
            "workers": 1,
            "warmup_repetitions": 1,
            "measured_repetitions": 3,
        },

        "dense": {
            "sizes": DENSE_SIZES,
            "methods": ["ijk", "ikj", "blocked"],
            "generation": (
                "C program generates two reproducible matrices A and B. "
                "Values are pseudo-random multiples of 0.001 between "
                "-1.000 and 1.000."
            ),
        },

        "sparse": {
            "sizes": SPARSE_SIZES,
            "zero_percentages": ZERO_PERCENTAGES,
            "density_percentages": [
                100 - percentage for percentage in ZERO_PERCENTAGES
            ],
            "generation": (
                "C program generates A with a requested percentage of "
                "zeros and nonzero candidate values drawn from multiples "
                "of 0.001 between -1.000 and 1.000. Matrix B is generated "
                "pseudo-randomly. The sparse program uses its own PRNG."
            ),
            "storage_format": "CSR",
        },
    }


def main():
    parser = argparse.ArgumentParser(
        description="Describe the input generation configuration."
    )
    parser.add_argument(
        "--output",
        default="results/generation_config.json",
        help="Output JSON configuration file.",
    )
    args = parser.parse_args()

    config = build_config()
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        json.dumps(config, indent=2),
        encoding="utf-8",
    )

    print("Generation configuration saved to:", output)
    print("Seed:", SEED)
    print("Dense sizes:", DENSE_SIZES)
    print("Sparse sizes:", SPARSE_SIZES)
    print("Requested zero percentages:", ZERO_PERCENTAGES)
    print("Note: the C programs generate the actual matrices internally.")


if __name__ == "__main__":
    main()
