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
    "cmake/EmrtcAdapterDiscovery.cmake",
    "README.md",
    "include/emrtc/adapter_contract.hpp",
    "include/emrtc/controller.hpp",
    "include/emrtc/axis.hpp",
    "include/emrtc/bus.hpp",
    "include/emrtc/hardware_adapters.hpp",
    "include/emrtc/multi_axis.hpp",
    "include/emrtc/process_image_mapping.hpp",
    "include/emrtc/scenario.hpp",
    "include/emrtc/watchdog.hpp",
    "src/controller.cpp",
    "src/axis.cpp",
    "src/adapter_contract.cpp",
    "src/bus.cpp",
    "src/hardware_adapters.cpp",
    "src/multi_axis.cpp",
    "src/process_image_mapping.cpp",
    "src/scenario.cpp",
    "src/watchdog.cpp",
    "apps/simulator_main.cpp",
    "apps/benchmark_main.cpp",
    "apps/diagnostic_cli_main.cpp",
    "tests/test_runner.cpp",
    "docs/architecture.md",
    "docs/bom.md",
    "docs/commissioning_logbook.md",
    "docs/demo_walkthrough.md",
    "docs/hardware_checkout.md",
    "docs/data_capture_profiles.md",
    "docs/recruiter_one_pager.md",
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
    "docs/adapters/exchange_contract.md",
    "docs/adapters/lifecycle_checklist.md",
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
    "reports/sample_two_axis_acceptance.svg",
    "reports/sample_capture_manifest.json",
    "reports/sample_capture_manifest.md",
    "config/capture_profiles/idle_bus_online.json",
    "config/capture_profiles/nominal_profile_position.json",
    "config/capture_profiles/cpu_jitter_stress.json",
    "config/capture_profiles/bus_dropout_recovery.json",
    "config/capture_profiles/heartbeat_timeout_recovery.json",
    "config/capture_profiles/encoder_loss_following_error.json",
    "examples/scenarios/fault_matrix.json",
    "examples/scenarios/heartbeat_timeout.json",
    "examples/scenarios/bus_disconnect.json",
    "examples/scenarios/command_abuse.json",
    "examples/scenarios/encoder_loss.json",
    "examples/scenarios/timing_stress.json",
    "scripts/generate_graphs.py",
    "scripts/generate_capture_manifest.py",
    "scripts/generate_one_pager.py",
    "scripts/demo_portfolio.ps1",
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

EXPECTED_CAPTURE_PROFILES = {
    "idle_bus_online",
    "nominal_profile_position",
    "cpu_jitter_stress",
    "bus_dropout_recovery",
    "heartbeat_timeout_recovery",
    "encoder_loss_following_error",
}

REQUIRED_CAPTURE_PROFILE_KEYS = {
    "profile_name",
    "purpose",
    "backend",
    "hardware_required",
    "cycle_time_ns",
    "duration_cycles",
    "preconditions",
    "commands",
    "fault_injections",
    "data_channels",
    "required_artifacts",
    "acceptance_criteria",
    "operator_notes",
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
    if not two_axis.get("acceptance_pass"):
        fail("sample metrics must include passing two-axis acceptance evidence")
    if two_axis.get("max_path_error", 999.0) > two_axis.get("path_error_limit", 0.0):
        fail("sample two-axis path error exceeds configured limit")
    if two_axis.get("final_position_error", 999.0) > two_axis.get("target_tolerance", 0.0):
        fail("sample two-axis final error exceeds configured tolerance")
    if two_axis.get("settling_time_s", 999.0) > two_axis.get("settling_time_limit_s", 0.0):
        fail("sample two-axis settling time exceeds configured limit")
    if not fault_log:
        fail("sample fault log must contain replayable entries")

    observed = {item["name"] for item in metrics["scenarios"]}
    missing = sorted(EXPECTED_SCENARIOS - observed)
    if missing:
        fail("sample metrics missing scenarios: " + ", ".join(missing))
    failed = [item["name"] for item in metrics["scenarios"] if item["result"] != "PASS"]
    if failed:
        fail("sample metrics contain failing scenarios: " + ", ".join(failed))


def check_capture_profiles() -> None:
    profile_dir = ROOT / "config/capture_profiles"
    paths = sorted(profile_dir.glob("*.json"))
    if len(paths) < len(EXPECTED_CAPTURE_PROFILES):
        fail("capture profile suite is too small")

    observed: set[str] = set()
    for path in paths:
        profile = json.loads(path.read_text(encoding="utf-8"))
        missing = REQUIRED_CAPTURE_PROFILE_KEYS - profile.keys()
        if missing:
            fail(f"{path.relative_to(ROOT)} missing keys: " + ", ".join(sorted(missing)))
        if profile["profile_name"] != path.stem:
            fail(f"{path.relative_to(ROOT)} profile_name must match filename")
        observed.add(profile["profile_name"])
        if not isinstance(profile["hardware_required"], bool):
            fail(f"{path.relative_to(ROOT)} hardware_required must be a boolean")
        if int(profile["cycle_time_ns"]) <= 0 or int(profile["duration_cycles"]) <= 0:
            fail(f"{path.relative_to(ROOT)} cycle_time_ns and duration_cycles must be positive")
        for key in ("preconditions", "commands", "fault_injections", "data_channels", "required_artifacts", "operator_notes"):
            if not isinstance(profile[key], list):
                fail(f"{path.relative_to(ROOT)} {key} must be a list")
        if not profile["required_artifacts"]:
            fail(f"{path.relative_to(ROOT)} required_artifacts must not be empty")
        criteria = profile["acceptance_criteria"]
        if not isinstance(criteria, dict) or not criteria:
            fail(f"{path.relative_to(ROOT)} acceptance_criteria must be a non-empty object")
        if "bench_extension" not in profile:
            fail(f"{path.relative_to(ROOT)} must include a bench_extension hardware boundary")

    missing_profiles = sorted(EXPECTED_CAPTURE_PROFILES - observed)
    if missing_profiles:
        fail("missing expected capture profiles: " + ", ".join(missing_profiles))

    manifest_path = ROOT / "reports/sample_capture_manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("profile_count") != len(paths):
        fail("sample capture manifest profile_count does not match profile suite")
    manifest_names = {profile["profile_name"] for profile in manifest.get("profiles", [])}
    if manifest_names != observed:
        fail("sample capture manifest does not match capture profile suite")


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
    check_capture_profiles()
    check_markdown_links()
    check_safety_language()
    print("PASS: repository health checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
