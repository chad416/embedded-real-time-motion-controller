---
name: Bench run evidence
about: Track one simulated or future hardware bench run from setup through evidence review
title: "Bench run: <target> / <capture profile>"
labels: bench-run, evidence
assignees: ""
---

## Scope

- Target profile:
- Capture profile:
- Backend: simulated / SOEM / EtherLab / SocketCAN / CANopenNode
- Software commit:
- Run date:
- Operator:

## Safety-Oriented Boundary

- [ ] This run does not claim certified SIL/PL safety.
- [ ] This run does not claim hardware validation unless the hardware evidence checklist below is complete.
- [ ] Motion limits, torque/current limits, travel limits, and stop path are documented before motion.

## Preconditions

- [ ] Local toolchain check completed or hosted CI commit is green.
- [ ] Hardware checkout checklist reviewed if this is a hardware run.
- [ ] Capture profile acceptance criteria copied into this issue.
- [ ] Drive/interface configuration recorded if this is a hardware run.

## Required Artifacts

- [ ] `trace.csv`
- [ ] `metrics.json`
- [ ] `fault_log.json`
- [ ] validation report or timing report
- [ ] relevant SVG/PDF plots
- [ ] operator notes
- [ ] CI run URL or local command transcript

## Hardware-Only Evidence

- [ ] Interface name and bus settings.
- [ ] Drive model, firmware, vendor/product IDs, or CANopen node ID.
- [ ] PDO/object dictionary mapping record.
- [ ] Drive parameter backup or exported configuration.
- [ ] Bench wiring/topology photos or screenshots.
- [ ] E-stop/STO/inhibit path notes.

## Result

- Pass/fail:
- Deviations:
- Follow-up issues:

## Claim Language

Use software simulation wording unless every relevant hardware-only evidence item is present and reviewed.
