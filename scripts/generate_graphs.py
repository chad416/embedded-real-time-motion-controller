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


def simulate_profile_position(cycles: int = 1200) -> list[tuple[float, float, float]]:
    position = 0.0
    velocity = 0.0
    target = 1.0
    dt = 0.001
    gain = 16.0
    max_velocity = 2.0
    max_accel = 40.0
    samples: list[tuple[float, float, float]] = []
    for cycle in range(cycles):
        error = target - position
        desired_velocity = max(-max_velocity, min(max_velocity, gain * error))
        accel_step = max_accel * dt
        velocity += max(-accel_step, min(accel_step, desired_velocity - velocity))
        position += velocity * dt
        if abs(error) < 0.0005 and abs(velocity) < 0.002:
            position = target
            velocity = 0.0
        if cycle % 8 == 0:
            samples.append((cycle * dt, position, velocity))
    return samples


def write_control_response(path: Path) -> None:
    samples = simulate_profile_position()
    width = 1000
    height = 420
    margin_left = 70
    margin_bottom = 60
    plot_width = width - margin_left - 40
    plot_height = height - 100 - margin_bottom
    max_time = samples[-1][0]

    def px(time_s: float) -> float:
        return margin_left + plot_width * time_s / max_time

    def py(position: float) -> float:
        return height - margin_bottom - plot_height * min(1.15, max(0.0, position)) / 1.15

    points = " ".join(f"{px(t):.1f},{py(p):.1f}" for t, p, _ in samples)
    target_y = py(1.0)
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        '<text x="30" y="36" font-family="Arial" font-size="24" font-weight="700" fill="#1f2937">Profile Position Step Response</text>',
        '<text x="30" y="62" font-family="Arial" font-size="13" fill="#4b5563">Simulated 1.0 unit absolute move with bounded velocity and acceleration</text>',
        f'<line x1="{margin_left}" y1="{height - margin_bottom}" x2="{width - 40}" y2="{height - margin_bottom}" stroke="#374151"/>',
        f'<line x1="{margin_left}" y1="100" x2="{margin_left}" y2="{height - margin_bottom}" stroke="#374151"/>',
        f'<line x1="{margin_left}" y1="{target_y:.1f}" x2="{width - 40}" y2="{target_y:.1f}" stroke="#16a34a" stroke-dasharray="6 4"/>',
        f'<text x="{width - 150}" y="{target_y - 8:.1f}" font-family="Arial" font-size="12" fill="#16a34a">target = 1.0</text>',
        f'<polyline fill="none" stroke="#2563eb" stroke-width="3" points="{points}"/>',
        '<text x="450" y="392" font-family="Arial" font-size="12" fill="#4b5563">time (s)</text>',
        '<text x="14" y="230" font-family="Arial" font-size="12" fill="#4b5563" transform="rotate(-90 14,230)">position</text>',
        '</svg>',
    ]
    path.write_text("\n".join(parts), encoding="utf-8")


def write_two_axis_path(path: Path) -> None:
    one_axis = simulate_profile_position()
    points_xy = [(p, p) for _, p, _ in one_axis]
    width = 700
    height = 700
    margin = 70
    plot = width - 2 * margin

    def px(x: float) -> float:
        return margin + plot * min(1.1, max(0.0, x)) / 1.1

    def py(y: float) -> float:
        return height - margin - plot * min(1.1, max(0.0, y)) / 1.1

    points = " ".join(f"{px(x):.1f},{py(y):.1f}" for x, y in points_xy)
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        '<text x="30" y="36" font-family="Arial" font-size="24" font-weight="700" fill="#1f2937">Two-Axis Coordinated Path</text>',
        '<text x="30" y="62" font-family="Arial" font-size="13" fill="#4b5563">Nominal straight-line XY move from (0,0) to (1,1)</text>',
        f'<rect x="{margin}" y="{margin + 20}" width="{plot}" height="{plot}" fill="#f9fafb" stroke="#d1d5db"/>',
        f'<line x1="{px(0):.1f}" y1="{py(0):.1f}" x2="{px(1):.1f}" y2="{py(1):.1f}" stroke="#16a34a" stroke-dasharray="6 4" stroke-width="2"/>',
        f'<polyline fill="none" stroke="#dc2626" stroke-width="3" points="{points}"/>',
        f'<circle cx="{px(1):.1f}" cy="{py(1):.1f}" r="6" fill="#111827"/>',
        f'<text x="{px(1) - 40:.1f}" y="{py(1) - 12:.1f}" font-family="Arial" font-size="12" fill="#111827">target</text>',
        '<text x="330" y="660" font-family="Arial" font-size="12" fill="#4b5563">X axis</text>',
        '<text x="18" y="350" font-family="Arial" font-size="12" fill="#4b5563" transform="rotate(-90 18,350)">Y axis</text>',
        '</svg>',
    ]
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
    write_control_response(out / "sample_control_response.svg")
    write_two_axis_path(out / "sample_two_axis_path.svg")
    print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
