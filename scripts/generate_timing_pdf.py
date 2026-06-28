#!/usr/bin/env python3
"""Generate a graph-first deterministic timing report PDF."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.pagesizes import letter
from reportlab.lib.styles import getSampleStyleSheet
from reportlab.platypus import Paragraph, SimpleDocTemplate, Spacer, Table, TableStyle


def load_metrics(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def table(data: list[list[object]]) -> Table:
    result = Table(data, hAlign="LEFT")
    result.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#1F4E79")),
                ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
                ("GRID", (0, 0), (-1, -1), 0.25, colors.HexColor("#B7B7B7")),
                ("FONTNAME", (0, 0), (-1, 0), "Helvetica-Bold"),
                ("FONTNAME", (0, 1), (-1, -1), "Helvetica"),
                ("FONTSIZE", (0, 0), (-1, -1), 8),
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, colors.HexColor("#F3F6FA")]),
            ]
        )
    )
    return result


def histogram_rows(metrics: dict) -> list[list[object]]:
    histogram = metrics.get("jitter_histogram", {})
    rows = [["Jitter bucket", "Cycles"]]
    for bucket in sorted(histogram, key=lambda item: int(item.replace("us", ""))):
        rows.append([bucket, histogram[bucket]])
    return rows


def scenario_rows(metrics: dict) -> list[list[object]]:
    rows = [["Scenario", "Result", "Recovery cycles"]]
    for item in metrics.get("scenarios", []):
        rows.append([item["name"], item["result"], item["recovery_cycles"]])
    return rows


def motion_rows(metrics: dict) -> list[list[object]]:
    motion = metrics.get("motion_performance", {})
    two_axis = metrics.get("two_axis_performance", {})
    return [
        ["Metric", "Value"],
        ["Target position", motion.get("target_position", 0.0)],
        ["Final position", motion.get("final_position", 0.0)],
        ["Overshoot", motion.get("overshoot", 0.0)],
        ["Settling time s", motion.get("settling_time_s", 0.0)],
        ["Steady-state error", motion.get("steady_state_error", 0.0)],
        ["Max following error", motion.get("max_abs_following_error", 0.0)],
        ["Two-axis RMS path error", two_axis.get("rms_path_error", 0.0)],
        ["Two-axis max path error", two_axis.get("max_path_error", 0.0)],
    ]


def build_pdf(metrics: dict, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    styles = getSampleStyleSheet()
    doc = SimpleDocTemplate(
        str(output),
        pagesize=letter,
        rightMargin=42,
        leftMargin=42,
        topMargin=42,
        bottomMargin=42,
        title="Deterministic Timing Report",
    )

    bus = metrics["bus"]
    story = [
        Paragraph("Deterministic Timing Report", styles["Title"]),
        Paragraph(
            "Software simulation evidence for the Embedded Real-Time Motion Controller. "
            "This report does not claim real hardware EtherCAT/CANopen validation or certified safety.",
            styles["BodyText"],
        ),
        Spacer(1, 12),
        Paragraph("Timing Summary", styles["Heading2"]),
        table(
            [
                ["Metric", "Value"],
                ["Cycle count", metrics["cycle_count"]],
                ["Min cycle ns", metrics["min_cycle_ns"]],
                ["Max cycle ns", metrics["max_cycle_ns"]],
                ["Mean cycle ns", metrics["mean_cycle_ns"]],
                ["Min jitter ns", metrics["min_jitter_ns"]],
                ["Max jitter ns", metrics["max_jitter_ns"]],
                ["Mean abs jitter ns", metrics["mean_abs_jitter_ns"]],
                ["Missed cycles", metrics["missed_cycles"]],
                ["Deadline overruns", metrics["deadline_overruns"]],
            ]
        ),
        Spacer(1, 12),
        Paragraph("Motion Performance", styles["Heading2"]),
        table(motion_rows(metrics)),
        Spacer(1, 12),
        Paragraph("Bus Diagnostics", styles["Heading2"]),
        table(
            [
                ["Counter", "Value"],
                ["TX packets", bus["tx_packets"]],
                ["RX packets", bus["rx_packets"]],
                ["Packet loss count", bus["packet_loss_count"]],
                ["Dropout count", bus["dropout_count"]],
                ["Invalid response count", bus["invalid_response_count"]],
                ["Heartbeat timeout count", bus["heartbeat_timeout_count"]],
                ["Cable disconnect events", bus["cable_disconnect_events"]],
            ]
        ),
        Spacer(1, 12),
        Paragraph("Jitter Distribution", styles["Heading2"]),
        table(histogram_rows(metrics)),
        Spacer(1, 12),
        Paragraph("Scenario Pass/Fail", styles["Heading2"]),
        table(scenario_rows(metrics)),
    ]
    doc.build(story)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--metrics", default="reports/sample_metrics.json")
    parser.add_argument("--out", default="reports/sample_timing_report.pdf")
    args = parser.parse_args()
    build_pdf(load_metrics(Path(args.metrics)), Path(args.out))
    print(args.out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
