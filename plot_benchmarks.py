#!/usr/bin/env python3
"""Turn a Google Benchmark CSV report into a self-contained SVG comparison."""

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


def parse_number(value):
    return float(value) if value not in (None, "") else None


def read_results(path):
    lines = path.read_text(encoding="utf-8").splitlines()
    try:
        header = next(index for index, line in enumerate(lines) if line.startswith("name,"))
    except StopIteration as error:
        raise ValueError("Could not find the CSV header (expected a line starting with 'name,')") from error

    rows = []
    for row in csv.DictReader(lines[header:]):
        match = NAME_PATTERN.match(row.get("name", ""))
        if not match or row.get("error_occurred") == "true":
            continue
        seconds = parse_number(row.get("cpu_time"))
        if seconds is None:
            continue
        unit = row.get("time_unit", "ns")
        if unit not in TIME_TO_SECONDS:
            raise ValueError(f"Unsupported time unit: {unit}")
        size = int(match.group("size"))
        seconds *= TIME_TO_SECONDS[unit]
        flops = parse_number(row.get("flop/s"))
        if flops is None:
            flops = (2.0 * size ** 3) / seconds
        rows.append({
            "implementation": match.group("implementation"),
            "size": size,
            "seconds": seconds,
            "gflops": flops / 1e9,
        })
    if not rows:
        raise ValueError("No matmul benchmark rows were found in the CSV")
    return rows


def log_slope(points):
    if len(points) < 2:
        return None
    xs = [math.log(point["size"]) for point in points]
    ys = [math.log(point["seconds"]) for point in points]
    x_mean, y_mean = sum(xs) / len(xs), sum(ys) / len(ys)
    denominator = sum((x - x_mean) ** 2 for x in xs)
    if denominator == 0:
        return None
    return sum((x - x_mean) * (y - y_mean) for x, y in zip(xs, ys)) / denominator


def svg_text(x, y, value, size=14, anchor="start", weight="normal", fill="#1f2937"):
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="Arial, sans-serif" '
            f'font-size="{size}" text-anchor="{anchor}" font-weight="{weight}" '
            f'fill="{fill}">{html.escape(value)}</text>')


def chart_frame(x, y, width, height, title, x_label, y_label):
    return [
        f'<rect x="{x}" y="{y}" width="{width}" height="{height}" fill="white" stroke="#cbd5e1"/>',
        svg_text(x + width / 2, y - 15, title, 18, "middle", "bold"),
        svg_text(x + width / 2, y + height + 42, x_label, 14, "middle"),
        f'<text x="{x - 48}" y="{y + height / 2}" font-family="Arial, sans-serif" font-size="14" '
        f'text-anchor="middle" fill="#1f2937" transform="rotate(-90 {x - 48} {y + height / 2})">{html.escape(y_label)}</text>',
    ]


def position(value, minimum, maximum, start, span, logarithmic=True):
    if maximum == minimum:
        return start + span / 2
    if logarithmic:
        value, minimum, maximum = math.log(value), math.log(minimum), math.log(maximum)
    return start + (value - minimum) / (maximum - minimum) * span


def add_line_chart(parts, grouped, sizes, value_key, x, y, width, height, title, y_label,
                   value_format, show_growth=False):
    values = [row[value_key] for rows in grouped.values() for row in rows]
    minimum, maximum = min(values), max(values)
    if minimum == maximum:
        minimum, maximum = minimum / 1.2, maximum * 1.2
    parts.extend(chart_frame(x, y, width, height, title, "Matrix dimension N", y_label))
    for size in sizes:
        x_pos = position(size, min(sizes), max(sizes), x, width)
        parts.append(f'<line x1="{x_pos:.1f}" y1="{y}" x2="{x_pos:.1f}" y2="{y + height}" stroke="#e2e8f0"/>')
        parts.append(svg_text(x_pos, y + height + 18, str(size), 12, "middle", fill="#475569"))
    for fraction in range(5):
        value = math.exp(math.log(minimum) + fraction / 4 * (math.log(maximum) - math.log(minimum)))
        y_pos = y + height - position(value, minimum, maximum, 0, height)
        parts.append(f'<line x1="{x}" y1="{y_pos:.1f}" x2="{x + width}" y2="{y_pos:.1f}" stroke="#e2e8f0"/>')
        parts.append(svg_text(x - 9, y_pos + 4, value_format(value), 12, "end", fill="#475569"))
    for index, (implementation, rows) in enumerate(grouped.items()):
        color = COLORS[index % len(COLORS)]
        points = []
        for row in rows:
            point_x = position(row["size"], min(sizes), max(sizes), x, width)
            point_y = y + height - position(row[value_key], minimum, maximum, 0, height)
            points.append((point_x, point_y))
        parts.append('<polyline fill="none" stroke="%s" stroke-width="2.5" points="%s"/>' %
                     (color, " ".join(f"{point_x:.1f},{point_y:.1f}" for point_x, point_y in points)))
        for point_x, point_y in points:
            parts.append(f'<circle cx="{point_x:.1f}" cy="{point_y:.1f}" r="4" fill="{color}"/>')
        slope = log_slope(rows) if show_growth else None
        label = implementation if slope is None else f"{implementation} (N^{slope:.2f})"
        legend_y = y + 18 + index * 18
        parts.append(f'<line x1="{x + width - 185}" y1="{legend_y - 4}" x2="{x + width - 165}" y2="{legend_y - 4}" stroke="{color}" stroke-width="2.5"/>')
        parts.append(svg_text(x + width - 160, legend_y, label, 12, fill="#334155"))


def write_svg(rows, output, title):
    grouped = defaultdict(list)
    for row in rows:
        grouped[row["implementation"]].append(row)
    grouped = dict(sorted(grouped.items()))
    for values in grouped.values():
        values.sort(key=lambda row: row["size"])
    sizes = sorted({row["size"] for row in rows})

    width, height = 1120, 1080
    chart_x, chart_width, chart_height = 125, 875, 225
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
             '<rect width="100%" height="100%" fill="#f8fafc"/>',
             svg_text(width / 2, 38, title, 24, "middle", "bold"),
             svg_text(width / 2, 62, "Logarithmic axes; growth exponent is fitted from CPU time", 14, "middle", fill="#475569")]
    add_line_chart(parts, grouped, sizes, "seconds", chart_x, 115, chart_width, chart_height,
                   "CPU time growth", "CPU time (seconds, log scale)",
                   lambda value: f"{value * 1e3:.2g} ms", show_growth=True)
    add_line_chart(parts, grouped, sizes, "gflops", chart_x, 445, chart_width, chart_height,
                   "Throughput by matrix size", "GFLOP/s (log scale)",
                   lambda value: f"{value:.2g}")

    naive = {row["size"]: row["seconds"] for row in grouped.get("naive", [])}
    speedup_grouped = {}
    for implementation, values in grouped.items():
        speedup = [{**row, "speedup": naive[row["size"]] / row["seconds"]}
                   for row in values if row["size"] in naive]
        if speedup:
            speedup_grouped[implementation] = speedup
    if speedup_grouped:
        add_line_chart(parts, speedup_grouped, sizes, "speedup", chart_x, 775, chart_width, chart_height,
                       "Speedup relative to naive", "Speedup × (log scale)", lambda value: f"{value:.2g}×")
    else:
        parts.append(svg_text(width / 2, 885, "Speedup chart requires a 'naive' baseline in the CSV", 16, "middle", fill="#475569"))
    parts.append("</svg>")
    output.write_text("\n".join(parts), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Plot matmul Google Benchmark CSV results as SVG.")
    parser.add_argument("csv", type=Path, help="CSV created by matmul_bench --csv=PATH")
    parser.add_argument("--output", type=Path, default=Path("benchmark-growth.svg"), help="SVG output path")
    parser.add_argument("--title", default="Matrix multiplication benchmark growth", help="Chart title")
    args = parser.parse_args()
    write_svg(read_results(args.csv), args.output, args.title)
    print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
