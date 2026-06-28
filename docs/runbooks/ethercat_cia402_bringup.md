# EtherCAT CiA 402 Bring-Up Runbook

Use this runbook after a real EtherCAT CiA 402 drive, motor, and low-energy bench setup have been selected.

## Target Profile

`config/hardware_targets/generic_ethercat_cia402_servo.json`

## Preconditions

- Hardware checkout checklist is complete.
- E-stop/STO/inhibit path is documented outside this software.
- Drive is configured for low speed, low torque/current, and limited travel.
- SOEM or EtherLab dependency discovery is recorded.
- A bench-run issue and adapter-commissioning issue are open.

## Sequence

1. Record host interface name and EtherCAT master path.
2. Scan slaves without enabling motion.
3. Record vendor/product IDs and drive firmware.
4. Export or document the PDO map.
5. Validate required CiA 402 objects:
   - `0x6040` controlword
   - `0x6041` statusword
   - `0x607A` target position
   - `0x60FF` target velocity or mode-specific equivalent
   - `0x6064` actual position
   - `0x606C` actual velocity
6. Run idle cyclic exchange.
7. Run `idle_bus_online`.
8. Run a reduced-distance `nominal_profile_position`.
9. Run `heartbeat_timeout_recovery` only if the test method is low risk.
10. Run `bus_dropout_recovery` only after confirming the physical stop path and low-energy limits.

## Required Artifacts

- Slave scan log.
- PDO mapping dump.
- Interface name and master configuration.
- Raw trace CSV.
- Metrics JSON.
- Fault log JSON.
- Timing report or timing plot.
- Commissioning logbook entry.
- Bench topology photo or screenshot.

## Stop Conditions

Stop immediately if any of the following occur:

- Unexpected motion direction.
- Position feedback sign is wrong.
- Statusword does not match expected CiA 402 sequence.
- Drive fault cannot be reset through the documented procedure.
- E-stop/STO/inhibit path is unclear.

## Boundary

Passing this runbook is bench commissioning evidence. It is not a certified safety claim.
