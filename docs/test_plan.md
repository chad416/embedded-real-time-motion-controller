# Test Plan

The C++ test runner is intentionally dependency-free and is registered with CTest.

## Automated Scenarios

| Scenario | Evidence |
| --- | --- |
| Normal startup | Boot reaches Switch On Disabled. |
| Drive enable sequence | Operation Enabled and statusword 0x0027. |
| Absolute move | Position settles near target. |
| Relative move | Relative displacement settles near target. |
| Velocity move | Velocity ramps toward command. |
| Quick stop during motion | Velocity decays to stop band. |
| E-stop/safe stop during motion | EmergencyStop fault record. |
| Fault reset and recovery | Reset returns to re-enable path. |
| Invalid state transition rejected | Fault code is InvalidDriveTransition. |
| Missed-cycle injection | Missed-cycle counter increments. |
| CPU load/jitter stress | Deadline-overrun counter increments. |
| Heartbeat timeout | Heartbeat watchdog trips. |
| Bus cable disconnect simulation | Disconnect counter and BusDropout fault. |
| Packet loss simulation | Packet-loss counter increments. |
| Encoder feedback loss | Encoder watchdog trips. |
| Following error | Following-error watchdog trips. |
| Command abuse during motion | Invalid command is rejected and logged. |
| Sensor noise injection | Bounded noise visible without false trip. |
| Warm restart | Re-enable succeeds while preserving position model. |
| Cold restart | Position model resets and re-enable succeeds. |
| Persistent fault log replay | Fault log contains cycle/time/fault record. |
| Hardware adapter skeleton status | Adapters stay uncommissioned even when optional dependency discovery is enabled. |
| CiA 402 process-image mapping contract | Object indexes and counts-per-unit scaling round trip. |

## Commands

```powershell
scripts/build.ps1
scripts/test.ps1
```

If the local machine lacks CMake or a C++17 compiler, install those tools and rerun. The Python report generator is only a sample-evidence fallback, not a replacement for native C++ tests.
