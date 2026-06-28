# Commissioning Logbook Template

This logbook is for future hardware commissioning. It is included now so the software project already has a disciplined bring-up path.

## Project Information

| Field | Value |
| --- | --- |
| Project | Embedded Real-Time Motion Controller |
| Bench ID | TBD |
| Commissioning engineer | TBD |
| Date | TBD |
| Hardware revision | TBD |
| Software commit | TBD |
| Drive/actuator model | TBD |
| Fieldbus backend | Simulated / SOEM / EtherLab / SocketCAN / CANopenNode |

## Pre-Power Checks

| Check | Expected Result | Pass/Fail | Notes |
| --- | --- | --- | --- |
| Wiring matches schematic | No undocumented field wiring | TBD | TBD |
| Protective earth verified | Continuity verified where applicable | TBD | TBD |
| 24 V DC polarity | Correct at terminal block and controller | TBD | TBD |
| Drive supply isolated | Supply off before logic checks | TBD | TBD |
| E-stop chain inspected | Contacts and safety relay wiring verified | TBD | TBD |
| Encoder wiring inspected | Shielding, polarity, connector seating verified | TBD | TBD |
| Fieldbus cable inspected | Cable type, order, shielding, strain relief verified | TBD | TBD |

## Bring-Up Sequence

| Step | Action | Expected Evidence | Result | Notes |
| ---: | --- | --- | --- | --- |
| 1 | Power logic supply only | Controller boots, no drive enable | TBD | TBD |
| 2 | Verify diagnostic shell | `show status` returns no active fault | TBD | TBD |
| 3 | Verify E-stop input | Safe-stop request visible in telemetry | TBD | TBD |
| 4 | Start fieldbus master | Bus online, heartbeat observed | TBD | TBD |
| 5 | Read drive state | CiA 402 state maps to Switch On Disabled | TBD | TBD |
| 6 | Enable drive without load motion | Operation Enabled, no unexpected motion | TBD | TBD |
| 7 | Jog low velocity | Direction, scale, current, and encoder sign verified | TBD | TBD |
| 8 | Run absolute move | Position, overshoot, settling logged | TBD | TBD |
| 9 | Pull fieldbus cable | Bus watchdog and recovery behavior logged | TBD | TBD |
| 10 | Trigger E-stop | Motion stops through intended safety path | TBD | TBD |

## Timing Evidence

| Scenario | Cycle Time Mean | Max Jitter | Missed Cycles | Deadline Overruns | Artifact |
| --- | ---: | ---: | ---: | ---: | --- |
| Idle bus online | TBD | TBD | TBD | TBD | TBD |
| Nominal move | TBD | TBD | TBD | TBD | TBD |
| CPU stress | TBD | TBD | TBD | TBD | TBD |
| Bus fault | TBD | TBD | TBD | TBD | TBD |

## Fault Recovery Evidence

| Fault | Injection Method | Expected Reaction | Recovery Step | Pass/Fail | Artifact |
| --- | --- | --- | --- | --- | --- |
| Missed cycle | Software injection | Timing counter increments | Continue or watchdog depending threshold | TBD | TBD |
| Heartbeat timeout | Stop heartbeat / disconnect device | Fault Reaction Active -> Fault | Restore bus, reset fault, re-enable | TBD | TBD |
| Encoder feedback loss | Disconnect/simulate encoder loss | Encoder watchdog trips | Restore feedback, reset fault | TBD | TBD |
| Following error | Load disturbance / software injection | Following-error fault | Remove load, reset fault | TBD | TBD |
| E-stop | Physical E-stop | Safe stop path activates | Release, reset safety chain, re-enable | TBD | TBD |

## Open Issues

| ID | Issue | Severity | Owner | Resolution |
| --- | --- | --- | --- | --- |
| TBD | TBD | TBD | TBD | TBD |

## Sign-Off

This logbook records engineering commissioning evidence only. It does not certify SIL/PL safety.

| Role | Name | Date | Signature |
| --- | --- | --- | --- |
| Commissioning engineer | TBD | TBD | TBD |
| Reviewer | TBD | TBD | TBD |
