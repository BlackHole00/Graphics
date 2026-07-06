#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
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


def category_sort_key(category: str) -> tuple[int, object]:
    try:
        return (0, int(category))
    except ValueError:
        try:
            return (0, float(category))
        except ValueError:
            return (1, category)


def read_category_values(csv_path: Path) -> dict[str, list[float]]:
    categories: dict[str, list[float]] = defaultdict(list)

    with csv_path.open(newline="") as handle:
        reader = csv.reader(handle)
        for row in reader:
            if len(row) < 2:
                continue

            try:
                value = float(row[1])
            except ValueError:
                continue

            category = row[-1] if len(row) >= 3 else "all"
            categories[category].append(value)

    return categories


def collect_rows(input_path: Path) -> list[dict[str, str]]:
    output_rows: list[dict[str, str]] = []

    csv_files = [input_path] if input_path.is_file() else sorted(input_path.glob("*.csv"))
    if not csv_files:
        raise SystemExit(f"No CSV files found in {input_path}")

    for csv_path in csv_files:
        categories = read_category_values(csv_path)
        for category in sorted(categories, key=category_sort_key):
            values = categories[category]
            if not values:
                continue

            output_rows.append(
                {
                    "file": csv_path.name,
                    "category": category,
                    "average": f"{mean(values):.6f}",
                    "median": f"{median(values):.6f}",
                    "p25": f"{percentile(values, 25):.6f}",
                    "p75": f"{percentile(values, 75):.6f}",
                }
            )

    return output_rows


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Calculate average, median, p25, and p75 per category for timing CSV files."
    )
    parser.add_argument(
        "path",
        nargs="?",
        default="timings",
        help="CSV file or directory containing CSV files (default: timings)",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="timing_stats.csv",
        help="Output CSV file (default: timing_stats.csv)",
    )
    args = parser.parse_args()

    input_path = Path(args.path)
    rows = collect_rows(input_path)

    with Path(args.output).open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=["file", "category", "average", "median", "p25", "p75"])
        writer.writeheader()
        writer.writerows(rows)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())