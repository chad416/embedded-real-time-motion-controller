# CANopenNode Adapter Plan

## Purpose

Use CANopenNode for a future embedded-side CANopen path, especially if the controller core is moved onto an MCU/RTOS target.

## Build Flag

```bash
cmake -S . -B build -DEMRTC_ENABLE_CANOPENNODE=ON
```

## Planned External Dependency

- CANopenNode source or packaged library.
- Target CAN driver integration.
- Object dictionary definition for the controller/drive interface.
- RTOS or bare-metal timer integration for CANopen processing.

## Embedded Integration Notes

- Keep the deterministic controller loop independent from CANopen stack internals.
- Use explicit mailboxes or process-image copies between CANopen callbacks and control logic.
- Avoid blocking SDO calls in the cyclic control path.
- Treat heartbeat and node guarding as supervision inputs to the watchdog layer.

## Current Boundary

`CanOpenNodeBackend` is a non-commissioned integration point. It intentionally reports `configured=false`.
