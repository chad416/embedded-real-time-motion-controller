# EtherLab EtherCAT Adapter Plan

## Purpose

Use EtherLab as the production-grade Linux EtherCAT master path for the same `IBusBackend` process-image contract.

## Build Flag

```bash
cmake -S . -B build -DEMRTC_ENABLE_ETHERLAB=ON
```

## Planned External Dependency

- EtherLab master installed on Linux.
- Master service configured for the selected NIC.
- User permissions for real-time process-data exchange.

## Bring-Up Sequence

1. Verify master service status.
2. Scan slaves and record vendor/product IDs.
3. Configure PDOs for CiA 402 control/status and motion setpoints.
4. Run an idle cyclic exchange with timing capture.
5. Enable drive through the existing CiA 402 state-machine sequence.
6. Run low-speed motion and fault-injection tests.

## Current Boundary

`EtherLabEtherCatBackend` is a non-commissioned integration point. It intentionally reports `configured=false`.
