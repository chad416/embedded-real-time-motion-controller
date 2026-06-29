#!/usr/bin/env python3
"""Enforce the v0.3.0 all-non-hardware software-ready release contract."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any

import validate_project


ROOT = Path(__file__).resolve().parents[1]
DEFINITION_PATH = ROOT / "config/milestones/software_ready_v0.3.0.json"


class Gate:
    def __init__(self) -> None:
        self.failures: list[str] = []

    def check(self, name: str, condition: bool, detail: str) -> None:
        if condition:
            print(f"PASS: {name} - {detail}")
        else:
            self.failures.append(f"{name}: {detail}")
            print(f"FAIL: {name} - {detail}", file=sys.stderr)

    def finish(self) -> None:
        if self.failures:
            print(
                f"FAIL: software-ready milestone has {len(self.failures)} failed gate(s)",
                file=sys.stderr,
            )
            raise SystemExit(1)
        print("PASS: software-ready milestone gates")


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def project_version() -> str:
    cmake_text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(
        r"project\s*\(\s*embedded_realtime_motion_controller\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)",
        cmake_text,
        flags=re.MULTILINE,
    )
    return match.group(1) if match else ""


def cpp_test_count() -> int:
    test_text = (ROOT / "tests/test_runner.cpp").read_text(encoding="utf-8")
    match = re.search(
        r"const std::vector<TestResult> results\s*=\s*\{(.*?)\};",
        test_text,
        flags=re.DOTALL,
    )
    return len(re.findall(r"\btest_[A-Za-z0-9_]+\s*\(\s*\)\s*,", match.group(1))) if match else 0


def main() -> int:
    validate_project.main()

    gate = Gate()
    definition = load_json(DEFINITION_PATH)
    metrics = load_json(ROOT / "reports/sample_metrics.json")
    capture_manifest = load_json(ROOT / "reports/sample_capture_manifest.json")
    hardware_manifest = load_json(ROOT / "reports/sample_hardware_target_manifest.json")

    version = str(definition["version"])
    tag = str(definition["tag"])
    gate.check("project version", project_version() == version, f"CMake project version must be {version}")
    gate.check(
        "tag identity",
        tag == f"v{version}-software-ready",
        "tag must match the versioned software-ready naming rule",
    )

    missing = [path for path in definition["required_files"] if not (ROOT / path).is_file()]
    gate.check(
        "release inputs",
        not missing,
        "all required files exist" if not missing else "missing " + ", ".join(missing),
    )

    check_count = cpp_test_count()
    gate.check(
        "C++ check count",
        check_count >= int(definition["minimum_cpp_checks"]),
        f"{check_count} registered checks, minimum {definition['minimum_cpp_checks']}",
    )

    scenarios = metrics.get("scenarios", [])
    failed_scenarios = [
        str(item.get("name", "<unnamed>"))
        for item in scenarios
        if item.get("result") != "PASS"
    ]
    gate.check(
        "scenario evidence",
        len(scenarios) >= int(definition["minimum_scenarios"]) and not failed_scenarios,
        f"{len(scenarios)} scenarios with no failures"
        if not failed_scenarios
        else "failing scenarios: " + ", ".join(failed_scenarios),
    )

    recovery = metrics.get("recovery_performance", {})
    limits = definition["recovery_limits"]
    recovery_ok = (
        bool(recovery.get("available"))
        and bool(recovery.get("fault_observed"))
        and bool(recovery.get("operation_reenabled"))
        and bool(recovery.get("post_recovery_motion_settled"))
        and int(recovery.get("detection_latency_cycles", 999))
        <= int(limits["detection_latency_cycles_max"])
        and int(recovery.get("recovery_cycles", 999))
        <= int(limits["recovery_cycles_max"])
        and float(recovery.get("settling_time_ratio", 999.0))
        <= float(limits["settling_time_ratio_max"])
        and float(recovery.get("post_recovery_final_error", 999.0))
        <= float(limits["post_recovery_final_error_max"])
    )
    gate.check(
        "fault recovery evidence",
        recovery_ok,
        "detection, re-enable, settling, and final-error limits pass",
    )

    gate.check(
        "capture profiles",
        int(capture_manifest.get("profile_count", 0))
        >= int(definition["minimum_capture_profiles"]),
        f"{capture_manifest.get('profile_count', 0)} profiles available",
    )
    gate.check(
        "hardware target planning",
        int(hardware_manifest.get("target_count", 0))
        >= int(definition["minimum_hardware_targets"])
        and all(target.get("purchase_recommendation") is False for target in hardware_manifest.get("targets", [])),
        f"{hardware_manifest.get('target_count', 0)} vendor-neutral targets available",
    )

    soem_config = load_json(ROOT / "config/adapters/soem_single_axis.example.json")
    can_config = load_json(ROOT / "config/adapters/socketcan_canopen_single_axis.example.json")
    gate.check(
        "runtime adapter examples",
        soem_config.get("backend") == "soem_ethercat"
        and can_config.get("backend") == "socketcan_canopen"
        and int(soem_config.get("schema_version", 0)) == 1
        and int(can_config.get("schema_version", 0)) == 1,
        "strict versioned SOEM and SocketCAN/CANopen examples are present",
    )

    workflow_text = (ROOT / ".github/workflows/ci.yml").read_text(encoding="utf-8")
    missing_ci = [fragment for fragment in definition["required_ci_fragments"] if fragment not in workflow_text]
    gate.check(
        "CI enforcement",
        not missing_ci,
        "all release paths are exercised" if not missing_ci else "missing CI fragments: " + ", ".join(missing_ci),
    )

    milestone_text = (ROOT / "docs/software_ready_milestone.md").read_text(encoding="utf-8")
    gate.check(
        "release identity documentation",
        tag in milestone_text and version in milestone_text,
        f"milestone document names {tag}",
    )
    gate.check(
        "host timing boundary",
        "host timing evidence is not target real-time evidence" in milestone_text.lower(),
        "host measurements are explicitly separated from target evidence",
    )
    gate.check(
        "hardware boundary",
        "only physical hardware commissioning remains" in milestone_text.lower(),
        "the release leaves only physical commissioning work",
    )

    claim_text = "\n".join(
        (ROOT / path).read_text(encoding="utf-8") for path in definition["claim_documents"]
    ).lower()
    stale_claims = [
        phrase for phrase in definition["forbidden_current_claims"] if phrase.lower() in claim_text
    ]
    gate.check(
        "current claim audit",
        not stale_claims,
        "no stale implementation claims" if not stale_claims else "stale phrases: " + ", ".join(stale_claims),
    )

    gate.finish()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
