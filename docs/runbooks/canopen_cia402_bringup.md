# CANopen CiA 402 Bring-Up Runbook

Use this runbook after a real CANopen CiA 402 drive, motor, and low-energy bench setup have been selected.

## Target Profile

`config/hardware_targets/generic_canopen_cia402_servo.json`

## Preconditions

- Hardware checkout checklist is complete.
- E-stop/STO/inhibit path is documented outside this software.
- CAN bitrate, termination, and node ID are known.
- SocketCAN or CANopenNode dependency path is recorded.
- A bench-run issue and adapter-commissioning issue are open.

## Sequence

1. Record CAN interface name, bitrate, termination, and node ID.
2. Confirm CAN interface is up before starting controller logic.
3. Record object dictionary excerpt for required CiA 402 objects.
4. Record PDO map or SDO fallback for:
   - `0x6040` controlword
   - `0x6041` statusword
   - `0x607A` target position
   - `0x60FF` target velocity or mode-specific equivalent
   - `0x6064` actual position
   - `0x606C` actual velocity
5. Run idle heartbeat supervision.
6. Run `idle_bus_online`.
7. Run a reduced-distance `nominal_profile_position`.
8. Run `heartbeat_timeout_recovery`.
9. Run `encoder_loss_following_error` only with a drive-approved low-risk method.

## Required Artifacts

- CAN interface configuration.
- Node ID and bitrate record.
- Object dictionary excerpt.
- PDO mapping record.
- Heartbeat timeout trace.
- Raw trace CSV.
- Metrics JSON.
- Fault log JSON.
- Commissioning logbook entry.

## Stop Conditions

Stop immediately if any of the following occur:

- CAN bus-off persists after wiring/termination checks.
- Node ID conflict is suspected.
- Heartbeat configuration does not match the documented timeout.
- Drive state cannot be read reliably.
- Motion direction or encoder sign is unexpected.

## Boundary

Passing this runbook is bench commissioning evidence. It is not a certified safety claim.
