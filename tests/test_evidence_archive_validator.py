#!/usr/bin/env python3
"""Tests for evidence archive integrity and claim-boundary validation."""

from __future__ import annotations

import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from validate_evidence_archive import validate_archive  # noqa: E402


FIXTURE = (
    ROOT
    / "tests/fixtures/evidence_archive"
    / "2026-06-29_simulated_loopback_target_nominal_profile_position_001"
)


class EvidenceArchiveValidatorTests(unittest.TestCase):
    def test_complete_simulation_fixture_passes(self) -> None:
        self.assertEqual(validate_archive(FIXTURE, require_complete=True), [])

    def test_corrupted_artifact_fails_digest_and_size(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            archive = Path(temporary) / FIXTURE.name
            shutil.copytree(FIXTURE, archive)
            with (archive / "raw/trace.csv").open("a", encoding="utf-8") as stream:
                stream.write("2,1000000,0\n")

            errors = validate_archive(archive, require_complete=True)
            self.assertTrue(any("byte count mismatch" in error for error in errors))
            self.assertTrue(any("SHA-256 mismatch" in error for error in errors))

    def test_simulation_archive_cannot_enable_hardware_claim(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            archive = Path(temporary) / FIXTURE.name
            shutil.copytree(FIXTURE, archive)
            manifest_path = archive / "RUN_MANIFEST.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["hardware_claim_allowed"] = True
            manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

            errors = validate_archive(archive, require_complete=True)
            self.assertTrue(
                any("simulation archives cannot allow a hardware claim" in error for error in errors)
            )

    def test_unlisted_file_fails_archive_completeness(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            archive = Path(temporary) / FIXTURE.name
            shutil.copytree(FIXTURE, archive)
            (archive / "unlisted.txt").write_text("not in manifest\n", encoding="utf-8")

            errors = validate_archive(archive, require_complete=True)
            self.assertTrue(any("unlisted archive files" in error for error in errors))

    def test_parent_path_traversal_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            archive = Path(temporary) / FIXTURE.name
            shutil.copytree(FIXTURE, archive)
            manifest_path = archive / "RUN_MANIFEST.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["artifacts"][0]["path"] = "../operator_notes.md"
            manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

            errors = validate_archive(archive, require_complete=True)
            self.assertTrue(any("safe forward-slash relative path" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
