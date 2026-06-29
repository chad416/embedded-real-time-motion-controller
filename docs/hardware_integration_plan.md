# Hardware Integration Plan

Hardware commissioning starts after software validation is passing.

## Phase 1: Bench Setup

- Select target MCU or industrial PC.
- Select one EtherCAT or CANopen drive and motor pair.
- Add an isolated E-stop/safe-stop input path appropriate for the device.
- Document power, grounding, encoder, and network topology.

## Phase 2: Adapter Bring-Up

- Commission the dependency-backed single-slave SOEM backend, or implement an alternative EtherLab/SocketCAN path if the selected target requires it.
- Supply the selected interface, timeout, slave count, engineering-unit scale, and verified PDO offsets.
- Map the existing process image to actual PDO/object dictionary entries.
- Validate statusword/controlword transitions against the real drive manual.
- Keep simulated backend available for regression tests.

Adapter-specific plans are in [Hardware Adapter Integration Notes](adapters/README.md), and the required initialization/exchange/fault/recovery sequence is in [Adapter Lifecycle Checklist](adapters/lifecycle_checklist.md).

## Phase 3: Timing Validation

- Replace deterministic synthetic timing with hardware timer or `CLOCK_MONOTONIC` captures.
- Record cycle histograms under idle, nominal load, and stress load.
- Verify watchdog thresholds against measured jitter and drive heartbeat behavior.

## Phase 4: Safety Review

- Treat this code as safety-oriented support logic only.
- Do not claim SIL/PL without certified components, documented process, hazard analysis, verification, and validation.
- Keep hard safety functions in certified drive safety inputs, safety PLC, or equivalent certified hardware.
