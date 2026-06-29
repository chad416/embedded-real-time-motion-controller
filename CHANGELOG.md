# Changelog

All notable project changes are recorded here.

## [Unreleased]

### Added

- Strict structured JSON parsing and schema-checked SOEM/CANopen runtime configuration.
- SocketCAN CANopen NMT, RPDO/TPDO, heartbeat, and transport-loss implementation.
- Wall-clock host timing probe and pinned FreeRTOS cyclic-task smoke path.
- Quantitative baseline-versus-recovery metrics and comparison graph.

- Pinned, checksum-verified portable Windows CMake and MinGW-w64 installer and runner.
- Committed local Windows native build, CTest, benchmark, and report-generation validation record.
- Evidence archive validator with digest, completeness, profile, review, and claim-boundary checks.
- Positive, corruption, unlisted-file, and forbidden-claim archive tests.
- Replay-backed `IBusBackend` with recorded nominal and fault/recovery process-image traces.
- CLI, demo, and C++ tests for replay expectations and controller-output mismatch rejection.
- Dependency-backed SOEM v2 single-slave lifecycle and cyclic process-data exchange.
- Bounded little-endian SOEM PDO codec, working-counter supervision, and configurable receive timeout.
- External `IBusBackend` controller injection with measured CiA 402 feedback and one-cycle fault reset.
- Hosted CI build/test path against a pinned upstream SOEM commit.
- External-backend and SOEM configuration/codec tests, bringing the native suite to 38 checks.

### Changed

- Scenario loading no longer relies on regex extraction.
- Newly observed axis faults are persisted even when latched before watchdog evaluation.

- Toolchain checks now discover the bundled Codex Python runtime used by report generation.
- The portfolio demo uses the same Python fallback and fails immediately on native command errors.
- Native PowerShell build, test, benchmark, and report scripts now fail immediately on child-process errors.
- Simulation-only bus fault injection is reported as unavailable when an external backend is active.

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

- The pinned portable Windows CMake/C++17 workflow is installed and validated.
- SOEM cyclic exchange is implemented but not physically commissioned; EtherLab, SocketCAN, and CANopenNode remain planned.
- No physical drive, wiring, stop-path, or real-time target evidence is claimed.
