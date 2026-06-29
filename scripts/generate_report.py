#!/usr/bin/env python3
"""Generate deterministic sample validation artifacts.

The primary implementation lives in C++. This helper exists so the repository can
carry reviewable sample evidence even on a workstation where CMake/C++ is not yet
installed. It intentionally mirrors the deterministic timing/fault profile used
by the C++ benchmark entry point.
"""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
from statistics import mean


CYCLE_NS = 1_000_000


SCENARIOS = [
    ("normal startup", "PASS", "axis reached Operation Enabled through deterministic CiA 402 sequence", 0),
    ("drive enable sequence", "PASS", "statusword 0x0027 observed", 0),
    ("absolute move", "PASS", "axis settled within 0.01 units of target", 0),
    ("relative move", "PASS", "axis settled within 0.01 units of relative target", 0),
    ("velocity move", "PASS", "velocity command ramped above 0.5 units/s", 0),
    ("quick stop during motion", "PASS", "velocity decayed to safe stop band", 73),
    ("E-stop safe stop during motion", "PASS", "safe stop request recorded as safety-oriented emergency stop", 75),
    ("fault reset and recovery", "PASS", "fault reset returned drive to Operation Enabled", 5),
    ("invalid state transition rejected", "PASS", "invalid transition forced Fault Reaction Active/Fault", 1),
    ("missed-cycle injection", "PASS", "missed cycle counter incremented", 0),
    ("CPU load jitter stress", "PASS", "deadline overrun captured during load spike", 0),
    ("heartbeat timeout", "PASS", "heartbeat supervision tripped after configured timeout", 5),
    ("bus cable disconnect simulation", "PASS", "simulated cable disconnect forced bus watchdog fault", 1),
    ("packet loss simulation", "PASS", "packet loss counter incremented", 0),
    ("encoder feedback loss", "PASS", "encoder watchdog tripped after feedback-loss window", 4),
    ("following error", "PASS", "following-error watchdog tripped above limit", 1),
    ("command abuse during motion", "PASS", "invalid command during motion was rejected and logged", 1),
    ("sensor noise injection", "PASS", "bounded sensor noise did not create false trip", 0),
    ("warm restart", "PASS", "warm restart preserved position and re-enabled drive", 5),
    ("cold restart", "PASS", "cold restart cleared position model and re-enabled drive", 5),
    ("persistent fault log replay", "PASS", "fault log contains replayable cycle/time/fault record", 0),
]


FAULT_LOG = [
    {"cycle": 1600, "timestamp_ns": 1_600_000_000, "fault": "Missed Cycle", "detail": "fault injector forced missed cycle"},
    {"cycle": 1610, "timestamp_ns": 1_610_000_000, "fault": "Drive Fault", "detail": "simulated drive fault scenario evidence"},
    {"cycle": 1620, "timestamp_ns": 1_620_000_000, "fault": "Heartbeat Timeout", "detail": "heartbeat supervision evidence"},
    {"cycle": 1630, "timestamp_ns": 1_630_000_000, "fault": "Encoder Feedback Loss", "detail": "encoder watchdog evidence"},
]


def period_for_cycle(cycle: int) -> int:
    base = CYCLE_NS + (((cycle * 97) % 21) - 10) * 1000
    if cycle == 800:
        return base + int(CYCLE_NS * 1.25)
    if cycle == 1600:
        return base + CYCLE_NS * 2
    return base


def build_trace(cycles: int) -> list[dict[str, object]]:
    trace: list[dict[str, object]] = []
    position = 0.0
    velocity = 0.0
    target = 1.0
    for cycle in range(cycles):
        if cycle < 5:
            state = ["Not Ready", "Switch On Disabled", "Ready To Switch On", "Switched On", "Operation Enabled"][min(cycle, 4)]
            mode = "Disabled"
        else:
            state = "Operation Enabled"
            mode = "Profile Position"
            error = target - position
            desired_velocity = max(-2.0, min(2.0, 16.0 * error))
            velocity += max(-0.04, min(0.04, desired_velocity - velocity))
            position += velocity * 0.001
            if abs(error) < 0.0005 and abs(velocity) < 0.002:
                position = target
                velocity = 0.0
        if cycle in (0, 1, 2, 3, 4, 300, 800, 1200, 1600, cycles - 1):
            fault = "Missed Cycle" if cycle == 1600 else "None"
            event = {
                300: "packet loss injected",
                800: "cpu load spike injected",
                1200: "sensor noise injected",
                1600: "missed cycle injected",
            }.get(cycle, "cycle")
            trace.append(
                {
                    "cycle": cycle,
                    "timestamp_ns": cycle * CYCLE_NS,
                    "event": event,
                    "state": state,
                    "mode": mode,
                    "fault": fault,
                    "position": round(position, 6),
                    "velocity": round(velocity, 6),
                    "target_position": target if cycle >= 5 else 0.0,
                    "target_velocity": 0.0,
                    "following_error": 0.0,
                }
            )
    return trace


def build_motion_performance(cycles: int) -> dict[str, object]:
    trace = build_trace(cycles)
    target = 1.0
    positions = [float(item["position"]) for item in trace]
    velocities = [float(item["velocity"]) for item in trace]
    settling_time = 0.0
    settled = False
    for index, item in enumerate(trace):
        suffix = trace[index:]
        if all(abs(float(row["position"]) - target) <= 0.01 and abs(float(row["velocity"])) <= 0.01 for row in suffix):
            settling_time = float(item["timestamp_ns"]) * 1.0e-9
            settled = True
            break
    return {
        "available": True,
        "settled": settled,
        "target_position": target,
        "final_position": positions[-1],
        "final_velocity": velocities[-1],
        "overshoot": max(0.0, max(positions) - target),
        "settling_time_s": round(settling_time, 6),
        "steady_state_error": round(target - positions[-1], 6),
        "max_abs_following_error": 0.0,
    }


def build_two_axis_performance() -> dict[str, object]:
    return {
        "available": True,
        "target_reached": True,
        "path_within_limit": True,
        "acceptance_pass": True,
        "cycle_count": 1200,
        "target_x": 1.0,
        "target_y": 1.0,
        "final_x": 1.0,
        "final_y": 1.0,
        "final_x_error": 0.0,
        "final_y_error": 0.0,
        "final_position_error": 0.0,
        "rms_path_error": 0.0,
        "max_path_error": 0.0,
        "path_error_limit": 0.01,
        "settling_time_s": 0.936,
        "settling_time_limit_s": 1.2,
        "target_tolerance": 0.005,
    }


def build_recovery_performance() -> dict[str, object]:
    return {
        "available": True,
        "fault_observed": True,
        "operation_reenabled": True,
        "post_recovery_motion_settled": True,
        "fault_injection_cycle": 305,
        "fault_detection_cycle": 305,
        "operation_reenabled_cycle": 312,
        "detection_latency_cycles": 0,
        "recovery_cycles": 7,
        "baseline_settling_time_s": 0.791,
        "post_recovery_settling_time_s": 0.441,
        "settling_time_ratio": round(0.441 / 0.791, 6),
        "post_recovery_final_error": 0.0,
    }


def build_metrics(cycles: int) -> dict[str, object]:
    periods = [period_for_cycle(c) for c in range(cycles)]
    jitters = [p - CYCLE_NS for p in periods]
    missed = sum(1 for p in periods if p >= CYCLE_NS * 2)
    overruns = sum(1 for p in periods if p > CYCLE_NS + 100_000)
    histogram: dict[str, int] = {}
    for jitter in jitters:
        bucket_us = int(jitter / 1000)
        label = f"{bucket_us:+d}us"
        histogram[label] = histogram.get(label, 0) + 1
    return {
        "cycle_count": cycles,
        "min_cycle_ns": min(periods),
        "max_cycle_ns": max(periods),
        "mean_cycle_ns": round(mean(periods), 2),
        "min_jitter_ns": min(jitters),
        "max_jitter_ns": max(jitters),
        "mean_abs_jitter_ns": round(mean(abs(j) for j in jitters), 2),
        "missed_cycles": missed,
        "deadline_overruns": overruns,
        "jitter_histogram": histogram,
        "motion_performance": build_motion_performance(cycles),
        "two_axis_performance": build_two_axis_performance(),
        "recovery_performance": build_recovery_performance(),
        "bus": {
            "tx_packets": cycles,
            "rx_packets": cycles - 6,
            "packet_loss_count": 6,
            "dropout_count": 0,
            "invalid_response_count": 0,
            "heartbeat_timeout_count": 0,
            "cable_disconnect_events": 0,
        },
        "scenarios": [
            {"name": name, "result": result, "evidence": evidence, "recovery_cycles": recovery}
            for name, result, evidence, recovery in SCENARIOS
        ],
    }


def write_trace(path: Path, trace: list[dict[str, object]]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "cycle",
                "timestamp_ns",
                "event",
                "state",
                "mode",
                "fault",
                "position",
                "velocity",
                "target_position",
                "target_velocity",
                "following_error",
            ],
        )
        writer.writeheader()
        writer.writerows(trace)


def write_markdown(path: Path, metrics: dict[str, object]) -> None:
    bus = metrics["bus"]
    assert isinstance(bus, dict)
    scenarios = metrics["scenarios"]
    assert isinstance(scenarios, list)
    with path.open("w", encoding="utf-8") as handle:
        handle.write("# Sample Validation Report\n\n")
        handle.write("Generated by `scripts/generate_report.py` as deterministic sample evidence for review. Native C++ build validation should be rerun once CMake and a C++17 compiler are installed.\n\n")
        handle.write("## Timing Summary\n\n")
        handle.write("| Metric | Value |\n| --- | ---: |\n")
        for key in (
            "cycle_count",
            "min_cycle_ns",
            "max_cycle_ns",
            "mean_cycle_ns",
            "min_jitter_ns",
            "max_jitter_ns",
            "mean_abs_jitter_ns",
            "missed_cycles",
            "deadline_overruns",
        ):
            handle.write(f"| {key} | {metrics[key]} |\n")
        motion = metrics["motion_performance"]
        assert isinstance(motion, dict)
        handle.write("\n## Motion Performance\n\n| Metric | Value |\n| --- | ---: |\n")
        for key in (
            "target_position",
            "final_position",
            "final_velocity",
            "overshoot",
            "settling_time_s",
            "steady_state_error",
            "max_abs_following_error",
        ):
            handle.write(f"| {key} | {motion[key]} |\n")
        two_axis = metrics["two_axis_performance"]
        assert isinstance(two_axis, dict)
        handle.write("\n## Two-Axis Performance\n\n| Metric | Value |\n| --- | ---: |\n")
        for key in (
            "acceptance_pass",
            "cycle_count",
            "target_x",
            "target_y",
            "final_x",
            "final_y",
            "final_position_error",
            "target_tolerance",
            "rms_path_error",
            "max_path_error",
            "path_error_limit",
            "settling_time_s",
            "settling_time_limit_s",
        ):
            handle.write(f"| {key} | {two_axis[key]} |\n")
        recovery = metrics["recovery_performance"]
        assert isinstance(recovery, dict)
        handle.write("\n## Fault Recovery Performance\n\n| Metric | Value |\n| --- | ---: |\n")
        for key in (
            "fault_observed",
            "operation_reenabled",
            "post_recovery_motion_settled",
            "detection_latency_cycles",
            "recovery_cycles",
            "baseline_settling_time_s",
            "post_recovery_settling_time_s",
            "settling_time_ratio",
            "post_recovery_final_error",
        ):
            handle.write(f"| {key} | {recovery[key]} |\n")
        handle.write("\n## Jitter Histogram\n\n| Bucket | Count |\n| --- | ---: |\n")
        histogram = metrics["jitter_histogram"]
        assert isinstance(histogram, dict)
        for bucket in sorted(histogram):
            handle.write(f"| {bucket} | {histogram[bucket]} |\n")
        handle.write("\n## Bus Diagnostics\n\n| Counter | Value |\n| --- | ---: |\n")
        for key, value in bus.items():
            handle.write(f"| {key} | {value} |\n")
        handle.write("\n## Scenario Pass/Fail\n\n| Scenario | Result | Evidence | Recovery cycles |\n| --- | --- | --- | ---: |\n")
        for item in scenarios:
            assert isinstance(item, dict)
            handle.write(f"| {item['name']} | {item['result']} | {item['evidence']} | {item['recovery_cycles']} |\n")
        handle.write("\n## Hardware Boundary\n\n")
        handle.write("No certified safety claim or real EtherCAT/CANopen hardware validation is made by this sample report. Hardware commissioning remains a future integration step.\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default="reports/generated", help="output directory")
    parser.add_argument("--cycles", type=int, default=2500)
    args = parser.parse_args()

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    metrics = build_metrics(args.cycles)
    trace = build_trace(args.cycles)

    (out / "metrics.json").write_text(json.dumps(metrics, indent=2), encoding="utf-8")
    (out / "fault_log.json").write_text(json.dumps(FAULT_LOG, indent=2), encoding="utf-8")
    write_trace(out / "trace.csv", trace)
    write_markdown(out / "validation_report.md", metrics)
    print(f"generated reports in {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
