#!/usr/bin/env python3
"""Generate a manifest from vendor-neutral hardware target profiles."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


REQUIRED_KEYS = {
    "target_name",
    "target_type",
    "purpose",
    "fieldbus",
    "hardware_required",
    "purchase_recommendation",
    "required_interfaces",
    "required_process_image_objects",
    "candidate_capture_profiles",
    "pre_purchase_questions",
    "commissioning_risks",
    "evidence_required",
    "operator_notes",
}


def load_target(path: Path) -> dict[str, Any]:
    target = json.loads(path.read_text(encoding="utf-8"))
    missing = REQUIRED_KEYS - target.keys()
    if missing:
        raise ValueError(f"{path}: missing keys: {', '.join(sorted(missing))}")
    if target["target_name"] != path.stem:
        raise ValueError(f"{path}: target_name must match file stem")
    if target["purchase_recommendation"] is not False:
        raise ValueError(f"{path}: target profiles must not be purchase recommendations")
    for key in (
        "required_interfaces",
        "required_process_image_objects",
        "candidate_capture_profiles",
        "pre_purchase_questions",
        "commissioning_risks",
        "evidence_required",
        "operator_notes",
    ):
        if not isinstance(target[key], list):
            raise ValueError(f"{path}: {key} must be a list")
    return target


def summarize(target: dict[str, Any]) -> dict[str, Any]:
    return {
        "target_name": target["target_name"],
        "target_type": target["target_type"],
        "fieldbus": target["fieldbus"],
        "hardware_required": target["hardware_required"],
        "purchase_recommendation": target["purchase_recommendation"],
        "capture_profile_count": len(target["candidate_capture_profiles"]),
        "evidence_required": target["evidence_required"],
        "primary_risks": target["commissioning_risks"][:3],
    }


def fmt(items: list[Any]) -> str:
    return "<br>".join(str(item) for item in items) if items else "None"


def write_markdown(path: Path, targets: list[dict[str, Any]]) -> None:
    with path.open("w", encoding="utf-8") as handle:
        handle.write("# Hardware Target Manifest\n\n")
        handle.write("Generated from `config/hardware_targets/*.json`. These records are vendor-neutral planning profiles, not purchase recommendations.\n\n")
        handle.write("| Target | Fieldbus | Hardware required | Capture profiles | Required evidence | Primary risks |\n")
        handle.write("| --- | --- | --- | ---: | --- | --- |\n")
        for target in targets:
            handle.write(
                f"| `{target['target_name']}` | {target['fieldbus']} | {target['hardware_required']} | "
                f"{target['capture_profile_count']} | {fmt(target['evidence_required'])} | {fmt(target['primary_risks'])} |\n"
            )
        handle.write("\n## Boundary\n\n")
        handle.write("These profiles help plan future bench work. They do not claim hardware validation and do not recommend a specific vendor, drive, or purchase.\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--targets", default="config/hardware_targets")
    parser.add_argument("--out", default="reports")
    args = parser.parse_args()

    target_dir = Path(args.targets)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    targets = [summarize(load_target(path)) for path in sorted(target_dir.glob("*.json"))]
    if not targets:
        raise SystemExit(f"no hardware target profiles found in {target_dir}")
    manifest = {
        "target_count": len(targets),
        "targets": targets,
    }
    (out_dir / "sample_hardware_target_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    write_markdown(out_dir / "sample_hardware_target_manifest.md", targets)
    print(f"generated hardware target manifest for {len(targets)} targets in {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
