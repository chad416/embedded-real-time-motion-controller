#!/usr/bin/env python3
"""Generate lightweight SVG validation graphics from metrics JSON."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def svg_escape(value: object) -> str:
    return str(value).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def write_jitter_histogram(metrics: dict, path: Path) -> None:
    histogram = metrics.get("jitter_histogram", {})
    buckets = sorted(histogram, key=lambda item: int(item.replace("us", "")))
    width = 1000
    height = 420
    margin_left = 70
    margin_bottom = 70
    plot_width = width - margin_left - 30
    plot_height = height - 90 - margin_bottom
    max_count = max(histogram.values()) if histogram else 1
    bar_width = max(4, plot_width / max(1, len(buckets)) - 3)

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        '<text x="30" y="36" font-family="Arial" font-size="24" font-weight="700" fill="#1f2937">Jitter Distribution</text>',
        '<text x="30" y="62" font-family="Arial" font-size="13" fill="#4b5563">Deterministic 1 ms control-loop benchmark, bucketed by microseconds</text>',
        f'<line x1="{margin_left}" y1="{height - margin_bottom}" x2="{width - 30}" y2="{height - margin_bottom}" stroke="#374151"/>',
        f'<line x1="{margin_left}" y1="90" x2="{margin_left}" y2="{height - margin_bottom}" stroke="#374151"/>',
    ]

    for i in range(5):
        y = 90 + plot_height * i / 4
        value = int(max_count * (4 - i) / 4)
        parts.append(f'<line x1="{margin_left}" y1="{y:.1f}" x2="{width - 30}" y2="{y:.1f}" stroke="#e5e7eb"/>')
        parts.append(f'<text x="20" y="{y + 4:.1f}" font-family="Arial" font-size="11" fill="#6b7280">{value}</text>')

    for index, bucket in enumerate(buckets):
        count = histogram[bucket]
        x = margin_left + index * (plot_width / max(1, len(buckets)))
        bar_height = plot_height * count / max_count
        y = height - margin_bottom - bar_height
        color = "#2563eb" if abs(int(bucket.replace("us", ""))) <= 20 else "#dc2626"
        parts.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{bar_width:.1f}" height="{bar_height:.1f}" fill="{color}"/>')
        if count <= 2 or index % 3 == 0:
            parts.append(f'<text x="{x + bar_width / 2:.1f}" y="{height - margin_bottom + 18}" font-family="Arial" font-size="9" text-anchor="middle" fill="#4b5563" transform="rotate(45 {x + bar_width / 2:.1f},{height - margin_bottom + 18})">{svg_escape(bucket)}</text>')

    parts.append('</svg>')
    path.write_text("\n".join(parts), encoding="utf-8")


def write_scenario_matrix(metrics: dict, path: Path) -> None:
    scenarios = metrics.get("scenarios", [])
    row_height = 28
    width = 1100
    height = 90 + row_height * len(scenarios)
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        '<text x="30" y="36" font-family="Arial" font-size="24" font-weight="700" fill="#1f2937">Scenario Pass/Fail Matrix</text>',
        '<text x="30" y="62" font-family="Arial" font-size="13" fill="#4b5563">Deterministic automated scenarios and recovery evidence</text>',
    ]

    y = 82
    for index, scenario in enumerate(scenarios):
        fill = "#f9fafb" if index % 2 == 0 else "#ffffff"
        status_color = "#16a34a" if scenario["result"] == "PASS" else "#dc2626"
        parts.append(f'<rect x="30" y="{y - 18}" width="{width - 60}" height="{row_height}" fill="{fill}" stroke="#e5e7eb"/>')
        parts.append(f'<circle cx="52" cy="{y - 3}" r="7" fill="{status_color}"/>')
        parts.append(f'<text x="70" y="{y + 1}" font-family="Arial" font-size="12" fill="#111827">{svg_escape(scenario["name"])}</text>')
        parts.append(f'<text x="430" y="{y + 1}" font-family="Arial" font-size="12" fill="{status_color}" font-weight="700">{svg_escape(scenario["result"])}</text>')
        parts.append(f'<text x="510" y="{y + 1}" font-family="Arial" font-size="12" fill="#374151">recovery cycles: {svg_escape(scenario["recovery_cycles"])}</text>')
        parts.append(f'<text x="680" y="{y + 1}" font-family="Arial" font-size="11" fill="#6b7280">{svg_escape(scenario["evidence"])}</text>')
        y += row_height

    parts.append('</svg>')
    path.write_text("\n".join(parts), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--metrics", default="reports/sample_metrics.json")
    parser.add_argument("--out", default="reports")
    args = parser.parse_args()
    metrics = json.loads(Path(args.metrics).read_text(encoding="utf-8"))
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    write_jitter_histogram(metrics, out / "sample_jitter_histogram.svg")
    write_scenario_matrix(metrics, out / "sample_scenario_matrix.svg")
    print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
