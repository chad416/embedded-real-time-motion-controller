---
name: Adapter commissioning
about: Track future SOEM, EtherLab, SocketCAN, or CANopenNode adapter bring-up
title: "Adapter commissioning: <backend> / <target>"
labels: adapter, commissioning
assignees: ""
---

## Adapter Scope

- Backend: SOEM / EtherLab / SocketCAN / CANopenNode
- Hardware target profile:
- Interface:
- Software commit:
- Operator:

## Dependency And Build

- [ ] `scripts/check_toolchain.ps1 -RequireCMake -RequireCompiler` passes on the commissioning machine.
- [ ] Adapter CMake flag enabled.
- [ ] Dependency discovery result recorded.
- [ ] Build/test run attached.

## Mapping Contract

- [ ] Required CiA 402 objects are present.
- [ ] Object index, sub-index, bit width, direction, and scale are recorded.
- [ ] `validate_adapter_mapping()` equivalent check passes or discrepancy is documented.
- [ ] Stale receive data behavior is tested.
- [ ] Invalid response behavior is tested.
- [ ] Bus-offline/restored-exchange behavior is tested.

## Runtime Lifecycle

- [ ] Discover interface and devices without enabling motion.
- [ ] Configure process-image mapping.
- [ ] Run idle cyclic exchange.
- [ ] Enable through CiA 402 state sequence.
- [ ] Run low-energy motion profile.
- [ ] Trigger one supervised fault path.
- [ ] Recover through explicit reset and re-enable.
- [ ] Shut down to a known non-motion state.

## Evidence

- [ ] Adapter status output.
- [ ] Slave scan / CAN node record.
- [ ] Mapping dump.
- [ ] Timing trace.
- [ ] Fault log.
- [ ] Commissioning logbook entry.

## Boundary

Passing this issue means adapter bring-up evidence exists. It does not create a certified safety claim.
