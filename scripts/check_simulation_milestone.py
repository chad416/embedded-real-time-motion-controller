#!/usr/bin/env python3
"""Enforce the versioned simulation-complete milestone definition."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any

import validate_project


ROOT = Path(__file__).resolve().parents[1]
DEFINITION_PATH = ROOT / "config/milestones/simulation_complete_v0.2.0.json"


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
                f"FAIL: simulation-complete milestone has {len(self.failures)} failed gate(s)",
                file=sys.stderr,
            )
            raise SystemExit(1)
        print("PASS: simulation-complete milestone gates")


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


def semantic_version(value: str) -> tuple[int, int, int]:
    match = re.fullmatch(r"([0-9]+)\.([0-9]+)\.([0-9]+)", value)
    return tuple(int(part) for part in match.groups()) if match else (0, 0, 0)


def main() -> int:
    validate_project.main()

    gate = Gate()
    definition = load_json(DEFINITION_PATH)
    metrics = load_json(ROOT / "reports/sample_metrics.json")
    capture_manifest = load_json(ROOT / "reports/sample_capture_manifest.json")
    hardware_manifest = load_json(ROOT / "reports/sample_hardware_target_manifest.json")

    version = str(definition["version"])
    tag = str(definition["tag"])
    current_version = project_version()
    gate.check(
        "project version compatibility",
        semantic_version(current_version) >= semantic_version(version),
        f"current CMake version {current_version or '<missing>'} must preserve the {version} baseline",
    )
    gate.check(
        "tag identity",
        tag == f"v{version}-simulation-complete",
        "tag must match the versioned simulation-complete naming rule",
    )

    missing = [
        path for path in definition["required_files"] if not (ROOT / path).is_file()
    ]
    gate.check(
        "release inputs",
        not missing,
        "all required files exist" if not missing else "missing " + ", ".join(missing),
    )

    scenarios = metrics.get("scenarios", [])
    failed_scenarios = [
        str(item.get("name", "<unnamed>"))
        for item in scenarios
        if item.get("result") != "PASS"
    ]
    gate.check(
        "scenario count",
        len(scenarios) >= int(definition["minimum_scenarios"]),
        f"{len(scenarios)} scenarios, minimum {definition['minimum_scenarios']}",
    )
    gate.check(
        "scenario results",
        not failed_scenarios,
        "all scenarios pass"
        if not failed_scenarios
        else "failing scenarios: " + ", ".join(failed_scenarios),
    )

    motion = metrics.get("motion_performance", {})
    motion_limits = definition["motion_limits"]
    gate.check(
        "single-axis evidence",
        bool(motion.get("available"))
        and (
            not motion_limits["settled_required"]
            or bool(motion.get("settled"))
        )
        and float(motion.get("overshoot", float("inf")))
        <= float(motion_limits["overshoot_max"]),
        "motion evidence is available, settled, and within overshoot limit",
    )

    two_axis = metrics.get("two_axis_performance", {})
    two_axis_limits = definition["two_axis_limits"]
    gate.check(
        "two-axis evidence",
        bool(two_axis.get("available"))
        and (
            not two_axis_limits["acceptance_required"]
            or bool(two_axis.get("acceptance_pass"))
        )
        and (
            not two_axis_limits["path_within_limit_required"]
            or bool(two_axis.get("path_within_limit"))
        ),
        "coordinated motion acceptance and path bounds pass",
    )

    capture_count = int(capture_manifest.get("profile_count", 0))
    gate.check(
        "capture profiles",
        capture_count >= int(definition["minimum_capture_profiles"]),
        f"{capture_count} profiles, minimum {definition['minimum_capture_profiles']}",
    )

    targets = hardware_manifest.get("targets", [])
    target_count = int(hardware_manifest.get("target_count", 0))
    recommendations = [
        str(target.get("target_name", "<unnamed>"))
        for target in targets
        if target.get("purchase_recommendation") is not False
    ]
    gate.check(
        "hardware targets",
        target_count >= int(definition["minimum_hardware_targets"]),
        f"{target_count} targets, minimum {definition['minimum_hardware_targets']}",
    )
    gate.check(
        "purchase boundary",
        not recommendations,
        "all hardware target profiles remain vendor-neutral and non-recommending",
    )

    workflow_text = (ROOT / ".github/workflows/ci.yml").read_text(encoding="utf-8")
    gate.check(
        "CI enforcement",
        "python3 scripts/check_simulation_milestone.py" in workflow_text,
        "GitHub Actions executes the milestone gate",
    )

    milestone_text = (ROOT / "docs/software_complete_milestone.md").read_text(
        encoding="utf-8"
    )
    release_text = (ROOT / "docs/release_notes_template.md").read_text(
        encoding="utf-8"
    )
    gate.check(
        "release identity documentation",
        tag in milestone_text and tag in release_text,
        f"milestone and release template name {tag}",
    )
    gate.check(
        "hardware boundary documentation",
        "does not claim hardware validation" in milestone_text.lower()
        and "hardware validation is not included" in release_text.lower(),
        "release documentation preserves the simulation-only boundary",
    )

    gate.finish()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
