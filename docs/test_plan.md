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
| Hardware adapter status | Adapters stay uncommissioned until explicit runtime configuration succeeds. |
| Strict JSON parser | Nested values, escapes, duplicate/trailing/invalid input, and typed access are bounded. |
| Runtime adapter configuration | Version, backend, interface, mapping, layout, scaling, COB-ID, node, and timeout rules are enforced without hardware access. |
| SOEM configuration guards | Invalid interface, map size, slave count, timeout, and object mapping fail before NIC access. |
| SOEM PDO byte codec | Output/input fields are bounded and encoded little-endian. |
| SocketCAN CANopen PDO codec | Split RPDO/TPDO fields are bounded and encoded little-endian. |
| SocketCAN lifecycle and heartbeat | NMT, transmit/receive, heartbeat, stale-data, and clean shutdown use an injected transport. |
| SocketCAN transport loss | Send/receive failures become offline diagnostics without requiring a CAN interface. |
| Host absolute schedule | Wall-clock probe samples use stable absolute deadlines and export capability state. |
| External CiA 402 sequence | Statusword feedback drives state and emits shutdown, switch-on, enable-operation, and fault-reset controlwords. |
| Injected external backend | Controller forwards output targets, accepts measured feedback, exposes diagnostics, and rejects simulated-bus-only APIs. |
| CiA 402 process-image mapping contract | Object indexes and counts-per-unit scaling round trip. |
| Adapter mapping validator rejects invalid object map | Bad PDO/object dictionary mapping is rejected. |
| Adapter exchange monitor detects stale PDO data | Stale receive data becomes a bus watchdog fault. |
| Adapter exchange monitor detects invalid response | Invalid adapter response becomes a contract fault. |
| Adapter exchange monitor accepts restored exchange | Bus-offline observation is followed by accepted fresh receive data. |

## Commands

```powershell
scripts/build.ps1
scripts/test.ps1
```

On Windows, `scripts/run_all_portable.ps1` uses the pinned portable CMake/MinGW-w64 toolchain. The Python report generator is only a sample-evidence fallback, not a replacement for native C++ tests.

The current runner registers 45 C++ checks. FreeRTOS adds a separate cyclic-task smoke target, and Python adds five evidence-archive validator tests.
