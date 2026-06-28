#!/usr/bin/env python3
"""Validate an EMRTC simulation or hardware evidence archive."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from datetime import datetime
from pathlib import Path, PurePosixPath
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
MANIFEST_NAME = "RUN_MANIFEST.json"
SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")
COMMIT_PATTERN = re.compile(r"^[0-9a-f]{40}$")
VERSION_PATTERN = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+$")
RUN_ID_PATTERN = re.compile(r"^[0-9]{4}-[0-9]{2}-[0-9]{2}_[a-z0-9_]+_[0-9]{3}$")


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_utc_timestamp(value: object) -> bool:
    if not isinstance(value, str) or not value.endswith("Z"):
        return False
    try:
        parsed = datetime.fromisoformat(value[:-1] + "+00:00")
    except ValueError:
        return False
    return parsed.utcoffset() is not None


def safe_artifact_path(value: object) -> PurePosixPath | None:
    if not isinstance(value, str) or "\\" in value:
        return None
    path = PurePosixPath(value)
    if path.is_absolute() or not path.parts:
        return None
    if any(part in {"", ".", ".."} for part in path.parts):
        return None
    return path


def add(errors: list[str], condition: bool, message: str) -> None:
    if not condition:
        errors.append(message)


def validate_archive(
    run_directory: Path,
    *,
    repository_root: Path = ROOT,
    require_complete: bool = False,
) -> list[str]:
    errors: list[str] = []
    run_directory = run_directory.resolve()
    manifest_path = run_directory / MANIFEST_NAME

    if not run_directory.is_dir():
        return [f"run directory does not exist: {run_directory}"]
    if not manifest_path.is_file():
        return [f"missing {MANIFEST_NAME}: {manifest_path}"]

    try:
        manifest = load_json(manifest_path)
    except (OSError, json.JSONDecodeError) as exc:
        return [f"cannot read {MANIFEST_NAME}: {exc}"]

    schema = load_json(
        repository_root / "config/hardware_evidence_run_manifest.schema.json"
    )
    for key in schema.get("required", []):
        add(errors, key in manifest, f"manifest missing required field: {key}")
    if errors:
        return errors

    add(
        errors,
        type(manifest.get("schema_version")) is int
        and manifest.get("schema_version") == 1,
        "schema_version must be 1",
    )
    run_id = manifest.get("run_id")
    add(
        errors,
        isinstance(run_id, str) and bool(RUN_ID_PATTERN.fullmatch(run_id)),
        "run_id does not match the archive naming convention",
    )
    add(
        errors,
        manifest.get("run_id") == run_directory.name,
        "run_id must match the run directory name",
    )
    add(
        errors,
        manifest.get("run_type")
        in {"simulation", "ethercat_hardware", "canopen_hardware"},
        "run_type is not supported",
    )
    add(
        errors,
        parse_utc_timestamp(manifest.get("started_at_utc")),
        "started_at_utc must be an ISO-8601 UTC timestamp ending in Z",
    )
    add(
        errors,
        isinstance(manifest.get("operator"), str) and bool(manifest["operator"].strip()),
        "operator must be a non-empty string",
    )
    add(
        errors,
        manifest.get("result") in {"PASS", "FAIL", "ABORTED", "INCOMPLETE"},
        "result is not supported",
    )
    deviations = manifest.get("deviations")
    add(
        errors,
        isinstance(deviations, list)
        and all(isinstance(item, str) for item in deviations),
        "deviations must be an array of strings",
    )

    software = manifest.get("software")
    add(errors, isinstance(software, dict), "software must be an object")
    if isinstance(software, dict):
        add(
            errors,
            bool(COMMIT_PATTERN.fullmatch(str(software.get("commit", "")))),
            "software.commit must be a full lowercase Git commit hash",
        )
        add(
            errors,
            isinstance(software.get("worktree_clean"), bool),
            "software.worktree_clean must be a boolean",
        )
        add(
            errors,
            bool(VERSION_PATTERN.fullmatch(str(software.get("version", "")))),
            "software.version must use major.minor.patch format",
        )
        add(
            errors,
            isinstance(software.get("build_type"), str)
            and bool(software["build_type"].strip()),
            "software.build_type must be a non-empty string",
        )

    environment = manifest.get("environment")
    add(errors, isinstance(environment, dict), "environment must be an object")
    if isinstance(environment, dict):
        for key in ("host", "operating_system", "runtime"):
            add(
                errors,
                isinstance(environment.get(key), str)
                and bool(environment[key].strip()),
                f"environment.{key} must be a non-empty string",
            )
        cycle_time = environment.get("cycle_time_ns")
        add(
            errors,
            isinstance(cycle_time, int)
            and not isinstance(cycle_time, bool)
            and cycle_time > 0,
            "environment.cycle_time_ns must be a positive integer",
        )

    target_name = str(manifest.get("target_profile", ""))
    profile_name = str(manifest.get("capture_profile", ""))
    target_path = repository_root / "config/hardware_targets" / f"{target_name}.json"
    profile_path = repository_root / "config/capture_profiles" / f"{profile_name}.json"
    add(errors, target_path.is_file(), f"unknown target profile: {target_name}")
    add(errors, profile_path.is_file(), f"unknown capture profile: {profile_name}")

    target = load_json(target_path) if target_path.is_file() else {}
    profile = load_json(profile_path) if profile_path.is_file() else {}
    if target:
        add(
            errors,
            target.get("target_name") == target_name,
            "target profile name does not match its filename",
        )
    if profile:
        add(
            errors,
            profile.get("profile_name") == profile_name,
            "capture profile name does not match its filename",
        )
    run_type = manifest.get("run_type")
    is_hardware = run_type in {"ethercat_hardware", "canopen_hardware"}
    add(
        errors,
        bool(target.get("hardware_required")) == is_hardware,
        "run_type and target_profile hardware boundary do not match",
    )
    add(
        errors,
        manifest.get("hardware_claim_allowed") is not True or is_hardware,
        "simulation archives cannot allow a hardware claim",
    )
    add(
        errors,
        isinstance(manifest.get("hardware_claim_allowed"), bool),
        "hardware_claim_allowed must be a boolean",
    )
    if run_type == "ethercat_hardware":
        add(errors, target.get("fieldbus") == "EtherCAT", "EtherCAT run requires an EtherCAT target")
    if run_type == "canopen_hardware":
        add(errors, target.get("fieldbus") == "CANopen", "CANopen run requires a CANopen target")

    artifacts = manifest.get("artifacts")
    add(errors, isinstance(artifacts, list), "artifacts must be an array")
    if not isinstance(artifacts, list):
        return errors

    listed_paths: set[str] = set()
    listed_basenames: set[str] = set()
    for index, artifact in enumerate(artifacts):
        label = f"artifacts[{index}]"
        if not isinstance(artifact, dict):
            errors.append(f"{label} must be an object")
            continue

        relative_path = safe_artifact_path(artifact.get("path"))
        if relative_path is None:
            errors.append(f"{label}.path must be a safe forward-slash relative path")
            continue

        path_text = relative_path.as_posix()
        add(errors, path_text not in listed_paths, f"duplicate artifact path: {path_text}")
        listed_paths.add(path_text)
        listed_basenames.add(relative_path.name)

        artifact_path = run_directory.joinpath(*relative_path.parts)
        try:
            artifact_path.resolve().relative_to(run_directory)
        except ValueError:
            errors.append(f"artifact escapes run directory: {path_text}")
            continue

        add(errors, artifact_path.is_file(), f"artifact file is missing: {path_text}")
        add(errors, not artifact_path.is_symlink(), f"artifact must not be a symlink: {path_text}")
        add(
            errors,
            isinstance(artifact.get("required"), bool),
            f"{label}.required must be a boolean",
        )
        add(
            errors,
            isinstance(artifact.get("role"), str)
            and bool(artifact["role"].strip()),
            f"{label}.role must be a non-empty string",
        )
        byte_count = artifact.get("bytes")
        add(
            errors,
            isinstance(byte_count, int)
            and not isinstance(byte_count, bool)
            and byte_count > 0,
            f"{label}.bytes must be a positive integer",
        )
        expected_hash = str(artifact.get("sha256", ""))
        add(
            errors,
            bool(SHA256_PATTERN.fullmatch(expected_hash)),
            f"{label}.sha256 must be a lowercase SHA-256 digest",
        )

        if artifact_path.is_file() and not artifact_path.is_symlink():
            actual_size = artifact_path.stat().st_size
            add(
                errors,
                artifact.get("bytes") == actual_size,
                f"artifact byte count mismatch: {path_text}",
            )
            if SHA256_PATTERN.fullmatch(expected_hash):
                add(
                    errors,
                    sha256_file(artifact_path) == expected_hash,
                    f"artifact SHA-256 mismatch: {path_text}",
                )

    actual_paths = {
        path.relative_to(run_directory).as_posix()
        for path in run_directory.rglob("*")
        if path.is_file() and path.name != MANIFEST_NAME
    }
    unlisted = sorted(actual_paths - listed_paths)
    missing_files = sorted(listed_paths - actual_paths)
    add(
        errors,
        not unlisted,
        "unlisted archive files: " + ", ".join(unlisted) if unlisted else "",
    )
    add(
        errors,
        not missing_files,
        "manifest lists missing files: " + ", ".join(missing_files)
        if missing_files
        else "",
    )

    claim_allowed = manifest.get("hardware_claim_allowed") is True
    complete = require_complete or manifest.get("result") == "PASS" or claim_allowed
    if complete:
        required_basenames = set(profile.get("required_artifacts", []))
        missing_required = sorted(required_basenames - listed_basenames)
        add(
            errors,
            not missing_required,
            "capture-profile artifacts missing: " + ", ".join(missing_required)
            if missing_required
            else "",
        )
        add(
            errors,
            "operator_notes.md" in listed_paths,
            "complete archives must list operator_notes.md",
        )
        add(
            errors,
            manifest.get("result") == "PASS",
            "complete archive result must be PASS",
        )

    hardware = manifest.get("hardware")
    add(errors, isinstance(hardware, list), "hardware must be an array")
    if isinstance(hardware, list):
        for index, item in enumerate(hardware):
            if not isinstance(item, dict):
                errors.append(f"hardware[{index}] must be an object")
                continue
            for key in ("role", "manufacturer", "model", "asset_id", "firmware"):
                add(
                    errors,
                    isinstance(item.get(key), str),
                    f"hardware[{index}].{key} must be a string",
                )
    if is_hardware:
        add(errors, bool(hardware), "hardware archives must identify bench hardware")
        if complete:
            add(
                errors,
                "config/mapping_dump.md" in listed_paths,
                "complete hardware archives must include config/mapping_dump.md",
            )

    review = manifest.get("review")
    add(errors, isinstance(review, dict), "review must be an object")
    if isinstance(review, dict):
        add(
            errors,
            review.get("status") in {"pending", "accepted", "rejected"},
            "review.status is not supported",
        )
        add(
            errors,
            isinstance(review.get("reviewer"), str),
            "review.reviewer must be a string",
        )
        reviewed_at = review.get("reviewed_at_utc")
        add(
            errors,
            reviewed_at is None or isinstance(reviewed_at, str),
            "review.reviewed_at_utc must be a string or null",
        )
    if claim_allowed:
        add(
            errors,
            manifest.get("result") == "PASS",
            "hardware claims require a PASS result",
        )
        add(
            errors,
            isinstance(review, dict) and review.get("status") == "accepted",
            "hardware claims require an accepted review",
        )
        add(
            errors,
            isinstance(review, dict) and bool(review.get("reviewer")),
            "hardware claims require a named reviewer",
        )
        add(
            errors,
            isinstance(review, dict)
            and parse_utc_timestamp(review.get("reviewed_at_utc")),
            "hardware claims require a UTC review timestamp",
        )
        add(
            errors,
            isinstance(software, dict) and software.get("worktree_clean") is True,
            "hardware claims require a clean source worktree",
        )
        add(
            errors,
            any(path.startswith("media/") for path in listed_paths),
            "hardware claims require at least one media artifact",
        )
        add(
            errors,
            any(path.startswith("config/drive_parameters/") for path in listed_paths),
            "hardware claims require a drive-parameter backup",
        )

    return [message for message in errors if message]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_directory", type=Path)
    parser.add_argument(
        "--require-complete",
        action="store_true",
        help="Require PASS status and all capture-profile artifacts.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    errors = validate_archive(
        args.run_directory,
        require_complete=args.require_complete,
    )
    if errors:
        for error in errors:
            print(f"FAIL: {error}", file=sys.stderr)
        print(f"FAIL: evidence archive has {len(errors)} error(s)", file=sys.stderr)
        return 1
    print(f"PASS: evidence archive {args.run_directory}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
