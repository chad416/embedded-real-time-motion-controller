#!/usr/bin/env python3
"""Generate a recruiter-facing one-pager from committed project evidence."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def pass_count(metrics: dict) -> tuple[int, int]:
    scenarios = metrics.get("scenarios", [])
    passed = sum(1 for item in scenarios if item.get("result") == "PASS")
    return passed, len(scenarios)


def write_one_pager(metrics: dict, manifest: dict, out: Path) -> None:
    passed, total = pass_count(metrics)
    motion = metrics.get("motion_performance", {})
    two_axis = metrics.get("two_axis_performance", {})
    bus = metrics.get("bus", {})
    profile_count = manifest.get("profile_count", 0)

    lines = [
        "# Embedded Real-Time Motion Controller - Portfolio One-Pager",
        "",
        "## Role Fit",
        "",
        "Industrial automation, robotics, embedded real-time software, fieldbus integration, controls validation, and commissioning-oriented engineering roles.",
        "",
        "## Project In One Sentence",
        "",
        "A C++17 software-first motion-controller validation bench with deterministic cyclic execution, CiA 402-style drive behavior, simulated EtherCAT/CANopen process image, watchdogs, fault injection, telemetry, visual reports, and explicit hardware commissioning boundaries.",
        "",
        "## Evidence Snapshot",
        "",
        "| Evidence | Value |",
        "| --- | ---: |",
        f"| Automated scenarios passing | {passed}/{total} |",
        f"| Benchmark cycles | {metrics.get('cycle_count', 0)} |",
        f"| Max jitter (ns) | {metrics.get('max_jitter_ns', 0)} |",
        f"| Missed cycles | {metrics.get('missed_cycles', 0)} |",
        f"| Deadline overruns | {metrics.get('deadline_overruns', 0)} |",
        f"| Packet-loss evidence | {bus.get('packet_loss_count', 0)} |",
        f"| Motion overshoot | {motion.get('overshoot', 0.0)} |",
        f"| Motion settling time (s) | {motion.get('settling_time_s', 0.0)} |",
        f"| Two-axis acceptance | {two_axis.get('acceptance_pass', False)} |",
        f"| Two-axis max path error | {two_axis.get('max_path_error', 0.0)} |",
        f"| Two-axis path limit | {two_axis.get('path_error_limit', 0.0)} |",
        f"| Capture profiles | {profile_count} |",
        "",
        "## Engineering Highlights",
        "",
        "- Deterministic 1 ms cyclic controller model with jitter, missed-cycle, and deadline instrumentation.",
        "- CiA 402-style axis state machine with command guards, statusword/controlword evidence, and fault reset behavior.",
        "- Simulated fieldbus backend covering packet loss, cable dropout, heartbeat timeout, and invalid responses.",
        "- Adapter contracts for CiA 402 object mapping, stale receive data, heartbeat health, bus-offline/restored-exchange behavior, and invalid responses.",
        "- Quantitative evidence: JSON metrics, CSV traces, fault logs, Markdown report, timing PDF, and SVG plots.",
        "- Repeatable portfolio demo script: `scripts/demo_portfolio.ps1`.",
        "",
        "## Best Interview Talking Points",
        "",
        "- How a timing anomaly becomes telemetry first, then watchdog evidence.",
        "- Why simulated bus faults are separated from future hardware claims.",
        "- How adapter contracts reduce risk before SOEM/EtherLab/SocketCAN/CANopenNode commissioning.",
        "- How capture profiles define pass/fail evidence for future bench runs.",
        "- How the same validation suite runs in hosted CI even before local Windows CMake/compiler setup is finished.",
        "",
        "## Honest Boundary",
        "",
        "This is a safety-oriented software simulation and host-validation project. It does not claim certified SIL/PL safety, real EtherCAT/CANopen hardware validation, real encoder measurements, or commissioned drive behavior.",
        "",
        "## Next Hardware Step",
        "",
        "Install local CMake plus a C++17 compiler, pick one EtherCAT or CANopen drive family, bind the process-image contract to real PDO/object dictionary entries, then capture the predefined profile evidence from `config/capture_profiles/`.",
        "",
    ]

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--metrics", default="reports/sample_metrics.json")
    parser.add_argument("--manifest", default="reports/sample_capture_manifest.json")
    parser.add_argument("--out", default="docs/recruiter_one_pager.md")
    args = parser.parse_args()

    metrics = load_json(Path(args.metrics))
    manifest = load_json(Path(args.manifest))
    write_one_pager(metrics, manifest, Path(args.out))
    print(args.out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
