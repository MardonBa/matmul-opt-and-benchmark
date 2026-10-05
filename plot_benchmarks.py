#!/usr/bin/env python3
"""Render selected matmul benchmark counters from Google Benchmark CSV as SVG."""

import argparse
import csv
import html
import math
import re
from collections import defaultdict
from pathlib import Path


NAME_PATTERN = re.compile(r"^matmul/(?P<implementation>[^/]+)/N:(?P<size>\d+)$")
TIME_TO_SECONDS = {"ns": 1e-9, "us": 1e-6, "ms": 1e-3, "s": 1.0}
COLORS = ["#2563eb", "#dc2626", "#16a34a", "#9333ea", "#ea580c", "#0891b2"]
METRICS = (
    ("cpu_seconds", "CPU time", "CPU time (ms, log scale)", lambda value: value * 1e3,
     lambda value: f"{value:.3g} ms"),
    ("cycles", "Cycles", "Cycles / multiplication (log scale)", lambda value: value,
     lambda value: f"{value / 1e6:.3g}M"),
    ("effective_bytes_per_second", "Effective bandwidth",
     "Effective bytes/s (GB/s, log scale)", lambda value: value / 1e9,
     lambda value: f"{value:.3g} GB/s"),
    ("flops_per_second", "Throughput", "FLOP/s (GFLOP/s, log scale)",
     lambda value: value / 1e9, lambda value: f"{value:.3g} GFLOP/s"),
    ("instructions", "Instructions", "Instructions / multiplication (log scale)",
     lambda value: value, lambda value: f"{value / 1e6:.3g}M"),
)


def parse_number(value):
    return float(value) if value not in (None, "") else None


def read_results(path):
    lines = path.read_text(encoding="utf-8").splitlines()
    try:
        header = next(index for index, line in enumerate(lines) if line.startswith("name,"))
    except StopIteration as error:
        raise ValueError("Could not find the CSV header (expected a line starting with 'name,')") from error

    rows = []
    missing_metrics = set()
    for row in csv.DictReader(lines[header:]):
        match = NAME_PATTERN.match(row.get("name", ""))
        if not match or row.get("error_occurred") == "true":
            continue
        cpu_time = parse_number(row.get("cpu_time"))
        time_unit = row.get("time_unit", "ns")
        if cpu_time is None or time_unit not in TIME_TO_SECONDS:
            continue
        result = {
            "implementation": match.group("implementation"),
            "size": int(match.group("size")),
            "cpu_seconds": cpu_time * TIME_TO_SECONDS[time_unit],
            "cycles": parse_number(row.get("cycles")),
            "effective_bytes_per_second": parse_number(row.get("effective_bytes/s")),
            "flops_per_second": parse_number(row.get("flop/s")),
            "instructions": parse_number(row.get("instructions")),
        }
        for key, *_ in METRICS:
            if result[key] is None:
                missing_metrics.add(key)
        rows.append(result)
    if not rows:
        raise ValueError("No matmul benchmark rows were found in the CSV")
    if missing_metrics:
        required = "cycles,instructions,effective-bandwidth,flops"
        raise ValueError(f"CSV is missing required counters ({', '.join(sorted(missing_metrics))}). "
                         f"Rerun with --metrics={required}.")
    return rows


def svg_text(x, y, value, size=14, anchor="start", weight="normal", fill="#1f2937"):
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="Arial, sans-serif" '
            f'font-size="{size}" text-anchor="{anchor}" font-weight="{weight}" '
            f'fill="{fill}">{html.escape(value)}</text>')


def log_position(value, minimum, maximum, start, span):
    if minimum == maximum:
        return start + span / 2
    return start + (math.log(value) - math.log(minimum)) / (math.log(maximum) - math.log(minimum)) * span


def chart_frame(x, y, width, height, title, y_label):
    return [
        f'<rect x="{x}" y="{y}" width="{width}" height="{height}" fill="white" stroke="#cbd5e1"/>',
        svg_text(x + width / 2, y - 14, title, 18, "middle", "bold"),
        svg_text(x + width / 2, y + height + 42, "Matrix dimension N", 14, "middle"),
        f'<text x="{x - 54}" y="{y + height / 2}" font-family="Arial, sans-serif" font-size="14" '
        f'text-anchor="middle" fill="#1f2937" transform="rotate(-90 {x - 54} {y + height / 2})">{html.escape(y_label)}</text>',
    ]


def add_line_chart(parts, grouped, sizes, key, chart_x, chart_y, chart_width,
                   chart_height, title, y_label, transform, format_value):
    values = [transform(row[key]) for rows in grouped.values() for row in rows]
    minimum, maximum = min(values), max(values)
    if minimum <= 0:
        raise ValueError(f"{key} contains a non-positive value and cannot be plotted on a log scale")
    if minimum == maximum:
        minimum /= 1.2
        maximum *= 1.2

    parts.extend(chart_frame(chart_x, chart_y, chart_width, chart_height, title, y_label))
    for size in sizes:
        x_pos = log_position(size, min(sizes), max(sizes), chart_x, chart_width)
        parts.append(f'<line x1="{x_pos:.1f}" y1="{chart_y}" x2="{x_pos:.1f}" y2="{chart_y + chart_height}" stroke="#e2e8f0"/>')
        parts.append(svg_text(x_pos, chart_y + chart_height + 18, str(size), 12, "middle", fill="#475569"))
    for fraction in range(5):
        value = math.exp(math.log(minimum) + fraction / 4 * (math.log(maximum) - math.log(minimum)))
        y_pos = chart_y + chart_height - log_position(value, minimum, maximum, 0, chart_height)
        parts.append(f'<line x1="{chart_x}" y1="{y_pos:.1f}" x2="{chart_x + chart_width}" y2="{y_pos:.1f}" stroke="#e2e8f0"/>')
        parts.append(svg_text(chart_x - 9, y_pos + 4, format_value(value), 12, "end", fill="#475569"))

    for index, (implementation, rows) in enumerate(grouped.items()):
        color = COLORS[index % len(COLORS)]
        points = []
        for row in rows:
            x_pos = log_position(row["size"], min(sizes), max(sizes), chart_x, chart_width)
            y_pos = chart_y + chart_height - log_position(
                transform(row[key]), minimum, maximum, 0, chart_height)
            points.append((x_pos, y_pos))
        parts.append('<polyline fill="none" stroke="%s" stroke-width="2.5" points="%s"/>' %
                     (color, " ".join(f"{x_pos:.1f},{y_pos:.1f}" for x_pos, y_pos in points)))
        for x_pos, y_pos in points:
            parts.append(f'<circle cx="{x_pos:.1f}" cy="{y_pos:.1f}" r="4" fill="{color}"/>')
        legend_y = chart_y + 18 + index * 18
        parts.append(f'<line x1="{chart_x + chart_width - 205}" y1="{legend_y - 4}" '
                     f'x2="{chart_x + chart_width - 185}" y2="{legend_y - 4}" '
                     f'stroke="{color}" stroke-width="2.5"/>')
        parts.append(svg_text(chart_x + chart_width - 180, legend_y, implementation, 12, fill="#334155"))


def write_svg(rows, output, title):
    grouped = defaultdict(list)
    for row in rows:
        grouped[row["implementation"]].append(row)
    grouped = dict(sorted(grouped.items()))
    for values in grouped.values():
        values.sort(key=lambda row: row["size"])
    sizes = sorted({row["size"] for row in rows})

    width, height = 1120, 1720
    chart_x, chart_width, chart_height = 145, 855, 220
    first_chart_y, chart_gap = 120, 105
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#f8fafc"/>',
        svg_text(width / 2, 38, title, 24, "middle", "bold"),
        svg_text(width / 2, 62, "Logarithmic axes; each color identifies one implementation", 14, "middle", fill="#475569"),
    ]
    for index, (key, chart_title, y_label, transform, format_value) in enumerate(METRICS):
        chart_y = first_chart_y + index * (chart_height + chart_gap)
        add_line_chart(parts, grouped, sizes, key, chart_x, chart_y, chart_width,
                       chart_height, chart_title, y_label, transform, format_value)
    parts.append("</svg>")
    output.write_text("\n".join(parts), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Plot selected matmul counters from a Google Benchmark CSV as SVG.")
    parser.add_argument("csv", type=Path, help="CSV created by matmul_bench --csv=PATH")
    parser.add_argument("--output", type=Path, default=Path("benchmark-metrics.svg"), help="SVG output path")
    parser.add_argument("--title", default="Matrix multiplication benchmark metrics", help="Chart title")
    args = parser.parse_args()
    write_svg(read_results(args.csv), args.output, args.title)
    print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
