# Changelog

All notable project changes are recorded here.

## [0.2.0] - 2026-06-29

### Added

- Deterministic C++17 controller, CiA 402-style axis, simulated process image, watchdogs, telemetry, CLI, benchmark, and test runner.
- JSON scenario suite with 21 passing nominal, timing, command-abuse, fault, and recovery cases.
- Coordinated two-axis simulation with explicit path, final-position, and settling acceptance limits.
- Adapter discovery, process-image mapping, and cyclic exchange contracts for future EtherCAT and CANopen backends.
- Deterministic Markdown, JSON, CSV, PDF, and SVG portfolio evidence.
- Versioned capture profiles, hardware target profiles, runbooks, issue templates, and commissioning records.
- Recruiter one-pager, demonstration walkthrough, and portfolio evidence bundle generation.
- Hardware evidence archive convention with a structured run-manifest schema.
- Machine-enforced simulation-complete milestone gate.

### Changed

- GitHub-maintained checkout and artifact-upload actions use the Node.js 24-based `v7` release lines.

### Validation

- GitHub Actions is the current native C++ build and test authority.
- Repository health and milestone checks enforce quantitative evidence and honest claim boundaries.

### Known Limitations

- The local Windows workstation still needs CMake and a C++17 compiler.
- Real SOEM, EtherLab, SocketCAN, or CANopenNode cyclic exchange is not commissioned.
- No physical drive, wiring, stop-path, or real-time target evidence is claimed.
