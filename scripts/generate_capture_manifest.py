#!/usr/bin/env python3
"""Generate a reviewable manifest from capture-profile JSON files."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


REQUIRED_KEYS = {
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


def fail(path: Path, message: str) -> None:
    raise ValueError(f"{path}: {message}")


def load_profile(path: Path) -> dict[str, Any]:
    profile = json.loads(path.read_text(encoding="utf-8"))
    missing = sorted(REQUIRED_KEYS - profile.keys())
    if missing:
        fail(path, "missing required keys: " + ", ".join(missing))
    if profile["profile_name"] != path.stem:
        fail(path, "profile_name must match the file stem")
    if not isinstance(profile["hardware_required"], bool):
        fail(path, "hardware_required must be a boolean")
    for key in ("preconditions", "commands", "fault_injections", "data_channels", "required_artifacts", "operator_notes"):
        if not isinstance(profile[key], list):
            fail(path, f"{key} must be a list")
    if not profile["required_artifacts"]:
        fail(path, "required_artifacts must not be empty")
    if not isinstance(profile["acceptance_criteria"], dict) or not profile["acceptance_criteria"]:
        fail(path, "acceptance_criteria must be a non-empty object")
    return profile


def summarize_profile(profile: dict[str, Any]) -> dict[str, Any]:
    criteria = profile["acceptance_criteria"]
    assert isinstance(criteria, dict)
    faults = profile["fault_injections"]
    assert isinstance(faults, list)
    return {
        "profile_name": profile["profile_name"],
        "purpose": profile["purpose"],
        "backend": profile["backend"],
        "hardware_required": profile["hardware_required"],
        "cycle_time_ns": profile["cycle_time_ns"],
        "duration_cycles": profile["duration_cycles"],
        "fault_injection_count": len(faults),
        "required_artifacts": profile["required_artifacts"],
        "acceptance_criteria": criteria,
        "bench_extension": profile.get("bench_extension", {}),
    }


def format_list(items: list[Any]) -> str:
    if not items:
        return "None"
    return ", ".join(str(item) for item in items)


def format_criteria(criteria: dict[str, Any]) -> str:
    return "<br>".join(f"`{key}` = `{value}`" for key, value in sorted(criteria.items()))


def write_markdown(path: Path, profiles: list[dict[str, Any]]) -> None:
    with path.open("w", encoding="utf-8") as handle:
        handle.write("# Capture Profile Manifest\n\n")
        handle.write("Generated from `config/capture_profiles/*.json`. These profiles define the evidence that must be collected during simulation and future hardware commissioning runs.\n\n")
        handle.write("This manifest does not claim real EtherCAT/CANopen hardware validation. Profiles with bench extensions describe future evidence requirements only.\n\n")
        handle.write("| Profile | Backend | Hardware required now | Cycles | Fault injections | Required artifacts | Acceptance criteria |\n")
        handle.write("| --- | --- | --- | ---: | ---: | --- | --- |\n")
        for profile in profiles:
            artifacts = format_list(profile["required_artifacts"])
            handle.write(
                f"| `{profile['profile_name']}` | {profile['backend']} | {profile['hardware_required']} | "
                f"{profile['duration_cycles']} | {profile['fault_injection_count']} | {artifacts} | "
                f"{format_criteria(profile['acceptance_criteria'])} |\n"
            )
        handle.write("\n## Hardware Boundary\n\n")
        handle.write("A profile can be simulation-ready while still carrying a bench-extension note. Hardware evidence must include raw traces, metrics, fault logs, timing plots, and operator notes before the run is described as hardware validated.\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profiles", default="config/capture_profiles", help="directory containing capture-profile JSON files")
    parser.add_argument("--out", default="reports", help="output directory for sample_capture_manifest files")
    args = parser.parse_args()

    profile_dir = Path(args.profiles)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    paths = sorted(profile_dir.glob("*.json"))
    if not paths:
        raise SystemExit(f"no capture profiles found in {profile_dir}")

    profiles = [summarize_profile(load_profile(path)) for path in paths]
    manifest = {
        "profile_count": len(profiles),
        "profiles": profiles,
    }
    (out_dir / "sample_capture_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    write_markdown(out_dir / "sample_capture_manifest.md", profiles)
    print(f"generated capture manifest for {len(profiles)} profiles in {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
