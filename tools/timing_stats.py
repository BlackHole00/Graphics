#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from statistics import mean, median


def percentile(values: list[float], percentile_value: float) -> float:
    if not values:
        raise ValueError("percentile() requires at least one value")

    if len(values) == 1:
        return values[0]

    ordered = sorted(values)
    position = (len(ordered) - 1) * (percentile_value / 100.0)
    lower_index = int(position)
    upper_index = min(lower_index + 1, len(ordered) - 1)
    fraction = position - lower_index

    return ordered[lower_index] * (1.0 - fraction) + ordered[upper_index] * fraction


def read_values(csv_path: Path) -> list[float]:
    values: list[float] = []

    with csv_path.open(newline="") as handle:
        reader = csv.reader(handle)
        for row in reader:
            if not row:
                continue
            if len(row) < 2:
                continue
            values.append(float(row[1]))

    return values[300:]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Calculate average, median, p25, and p75 for each CSV file in a folder."
    )
    parser.add_argument(
        "directory",
        nargs="?",
        default="timings",
        help="Directory containing the CSV files (default: timings)",
    )
    args = parser.parse_args()

    timings_dir = Path(args.directory)
    csv_files = sorted(timings_dir.glob("*.csv"))

    if not csv_files:
        raise SystemExit(f"No CSV files found in {timings_dir}")

    print("file,average,median,p25,p75")
    for csv_path in csv_files:
        values = read_values(csv_path)
        if not values:
            continue
        print(
            f"{csv_path.name},"
            f"{mean(values):.6f},"
            f"{median(values):.6f},"
            f"{percentile(values, 25):.6f},"
            f"{percentile(values, 75):.6f}"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())