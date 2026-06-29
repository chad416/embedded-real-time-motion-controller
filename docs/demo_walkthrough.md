# Portfolio Demo Walkthrough

This walkthrough is the repeatable demo path for the project. It is meant for portfolio reviews, interview preparation, and regression evidence.

## One Command

After the native build exists:

```powershell
scripts/demo_portfolio.ps1 -BuildDir build -OutDir reports/generated/demo
```

To package the review evidence after generating reports:

```powershell
scripts/package_portfolio.ps1 -OutDir reports/generated/portfolio_bundle
```

If the machine does not yet have CMake/C++ installed, the same script can still generate Python-based sample evidence:

```powershell
scripts/demo_portfolio.ps1 -SkipNative -OutDir reports/generated/demo
```

## What The Demo Shows

| Step | Evidence |
| --- | --- |
| Diagnostic status | Drive state, mode, position, velocity, and fault status from the CLI. |
| Adapter status and config dry-run | Implemented SOEM/SocketCAN paths, optional alternatives, strict mapping/configuration, and commissioning boundary. |
| Two-axis coordinated motion | Final X/Y position, RMS path error, max path error, final position error, settling time, and pass/fail thresholds. |
| Recorded adapter exchange | Stale data, bus offline, heartbeat loss, invalid response, and restoration checked against expected contract faults. |
| Fault-matrix scenario | JSON scenario loading plus packet loss, CPU jitter, sensor noise, and missed-cycle evidence. |
| Deterministic benchmark | Timing, bus, motion, and two-axis metrics. |
| Host timing probe | Wall-clock absolute-deadline scheduling, stress, and capability-report evidence. |
| Sample report generation | CSV, JSON, fault log, and validation report artifacts. |
| Capture manifest generation | Versioned evidence requirements for simulation and future bench profiles. |

## Talk Track

Start with the safety boundary: this is a safety-oriented software validation bench, not a certified safety product and not a hardware-validated EtherCAT/CANopen claim.

Then show the system behavior:

1. The CLI proves the controller can expose diagnostic state.
2. Adapter status proves hardware integration is isolated and honest about commissioning state.
3. The replay run proves recorded process-image traffic can challenge outgoing commands and fault/recovery expectations.
4. The scenario run proves controller fault injection is data-driven.
5. The benchmark proves timing, watchdog, and motion metrics are exported.
6. The host timing probe proves measured scheduling instrumentation while keeping target claims separate.
7. The capture manifest proves future bench runs have predefined acceptance criteria.

The best interview angle is not "it simulates a motor." The stronger angle is: the project defines control behavior, failure behavior, evidence artifacts, and the hardware boundary clearly enough that a reviewer can challenge each layer.
