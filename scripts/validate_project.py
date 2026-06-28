#!/usr/bin/env python3
"""Repository health checks for portfolio-grade validation evidence."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "CMakeLists.txt",
    "README.md",
    "include/emrtc/controller.hpp",
    "include/emrtc/axis.hpp",
    "include/emrtc/bus.hpp",
    "include/emrtc/hardware_adapters.hpp",
    "include/emrtc/multi_axis.hpp",
    "include/emrtc/scenario.hpp",
    "include/emrtc/watchdog.hpp",
    "src/controller.cpp",
    "src/axis.cpp",
    "src/bus.cpp",
    "src/hardware_adapters.cpp",
    "src/multi_axis.cpp",
    "src/scenario.cpp",
    "src/watchdog.cpp",
    "apps/simulator_main.cpp",
    "apps/benchmark_main.cpp",
    "apps/diagnostic_cli_main.cpp",
    "tests/test_runner.cpp",
    "docs/architecture.md",
    "docs/bom.md",
    "docs/commissioning_logbook.md",
    "docs/hardware_checkout.md",
    "docs/timing_budget.md",
    "docs/portfolio_alignment.md",
    "docs/technical_article.md",
    "docs/control_loop_architecture.md",
    "docs/bus_map.md",
    "docs/drive_state_machine.md",
    "docs/watchdog_design.md",
    "docs/fault_handling_matrix.md",
    "docs/test_plan.md",
    "docs/validation_report.md",
    "docs/benchmark_appendix.md",
    "docs/design_tradeoffs.md",
    "docs/hardware_integration_plan.md",
    "docs/adapters/README.md",
    "docs/adapters/soem_ethercat.md",
    "docs/adapters/etherlab_ethercat.md",
    "docs/adapters/socketcan_canopen.md",
    "docs/adapters/canopennode.md",
    "docs/final_report.md",
    "docs/demo_narratives.md",
    "reports/sample_metrics.json",
    "reports/sample_fault_log.json",
    "reports/sample_trace.csv",
    "reports/sample_validation_report.md",
    "reports/sample_timing_report.pdf",
    "reports/sample_jitter_histogram.svg",
    "reports/sample_scenario_matrix.svg",
    "reports/sample_control_response.svg",
    "reports/sample_two_axis_path.svg",
    "examples/scenarios/fault_matrix.json",
    "examples/scenarios/heartbeat_timeout.json",
    "examples/scenarios/bus_disconnect.json",
    "examples/scenarios/command_abuse.json",
    "examples/scenarios/encoder_loss.json",
    "examples/scenarios/timing_stress.json",
    "scripts/generate_graphs.py",
]

EXPECTED_SCENARIOS = {
    "normal startup",
    "drive enable sequence",
    "absolute move",
    "relative move",
    "velocity move",
    "quick stop during motion",
    "E-stop safe stop during motion",
    "fault reset and recovery",
    "invalid state transition rejected",
    "missed-cycle injection",
    "CPU load jitter stress",
    "heartbeat timeout",
    "bus cable disconnect simulation",
    "packet loss simulation",
    "encoder feedback loss",
    "following error",
    "command abuse during motion",
    "sensor noise injection",
    "warm restart",
    "cold restart",
    "persistent fault log replay",
}


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def check_required_files() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    if missing:
        fail("missing required files: " + ", ".join(missing))


def check_json_reports() -> None:
    metrics_path = ROOT / "reports/sample_metrics.json"
    fault_log_path = ROOT / "reports/sample_fault_log.json"
    metrics = json.loads(metrics_path.read_text(encoding="utf-8"))
    fault_log = json.loads(fault_log_path.read_text(encoding="utf-8"))

    if metrics["cycle_count"] < 1000:
        fail("sample metrics must include at least 1000 cycles")
    if metrics["missed_cycles"] < 1:
        fail("sample metrics must include missed-cycle evidence")
    if metrics["bus"]["packet_loss_count"] < 1:
        fail("sample metrics must include packet-loss evidence")
    motion = metrics.get("motion_performance", {})
    if not motion.get("available"):
        fail("sample metrics must include motion performance evidence")
    if not motion.get("settled"):
        fail("sample metrics must show the profile-position move settled")
    if motion.get("overshoot", 999.0) > 0.02:
        fail("sample motion overshoot is outside expected bound")
    two_axis = metrics.get("two_axis_performance", {})
    if not two_axis.get("available") or not two_axis.get("path_within_limit"):
        fail("sample metrics must include bounded two-axis path evidence")
    if not fault_log:
        fail("sample fault log must contain replayable entries")

    observed = {item["name"] for item in metrics["scenarios"]}
    missing = sorted(EXPECTED_SCENARIOS - observed)
    if missing:
        fail("sample metrics missing scenarios: " + ", ".join(missing))
    failed = [item["name"] for item in metrics["scenarios"] if item["result"] != "PASS"]
    if failed:
        fail("sample metrics contain failing scenarios: " + ", ".join(failed))


def check_markdown_links() -> None:
    markdown_files = sorted((ROOT / "docs").glob("*.md")) + [ROOT / "README.md"]
    pattern = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")
    for path in markdown_files:
        text = path.read_text(encoding="utf-8")
        for _, target in pattern.findall(text):
            if "://" in target or target.startswith("#"):
                continue
            relative = target.split("#", 1)[0]
            if not relative:
                continue
            resolved = (path.parent / relative).resolve()
            try:
                resolved.relative_to(ROOT)
            except ValueError:
                fail(f"{path.relative_to(ROOT)} links outside repo: {target}")
            if not resolved.exists():
                fail(f"{path.relative_to(ROOT)} has broken link: {target}")


def check_safety_language() -> None:
    text = "\n".join(path.read_text(encoding="utf-8") for path in (ROOT / "docs").glob("*.md"))
    text += "\n" + (ROOT / "README.md").read_text(encoding="utf-8")
    forbidden_claims = ["SIL certified", "PL certified", "hardware validated EtherCAT", "hardware validated CANopen"]
    for phrase in forbidden_claims:
        if phrase.lower() in text.lower():
            fail(f"forbidden certification/hardware claim found: {phrase}")
    if "safety-oriented" not in text:
        fail("documentation must use safety-oriented language")


def main() -> int:
    check_required_files()
    check_json_reports()
    check_markdown_links()
    check_safety_language()
    print("PASS: repository health checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
