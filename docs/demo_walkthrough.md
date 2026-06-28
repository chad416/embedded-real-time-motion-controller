# Portfolio Demo Walkthrough

This walkthrough is the repeatable demo path for the project. It is meant for portfolio reviews, interview preparation, and regression evidence.

## One Command

After the native build exists:

```powershell
scripts/demo_portfolio.ps1 -BuildDir build -OutDir reports/generated/demo
```

If the machine does not yet have CMake/C++ installed, the same script can still generate Python-based sample evidence:

```powershell
scripts/demo_portfolio.ps1 -SkipNative -OutDir reports/generated/demo
```

## What The Demo Shows

| Step | Evidence |
| --- | --- |
| Diagnostic status | Drive state, mode, position, velocity, and fault status from the CLI. |
| Adapter status | SOEM, EtherLab, SocketCAN, and CANopenNode discovery/configuration boundary. |
| Two-axis coordinated motion | Final X/Y position, RMS path error, max path error, final position error, settling time, and pass/fail thresholds. |
| Fault-matrix scenario | JSON scenario loading plus packet loss, CPU jitter, sensor noise, and missed-cycle evidence. |
| Deterministic benchmark | Timing, bus, motion, and two-axis metrics. |
| Sample report generation | CSV, JSON, fault log, and validation report artifacts. |
| Capture manifest generation | Versioned evidence requirements for simulation and future bench profiles. |

## Talk Track

Start with the safety boundary: this is a safety-oriented software validation bench, not a certified safety product and not a hardware-validated EtherCAT/CANopen claim.

Then show the system behavior:

1. The CLI proves the controller can expose diagnostic state.
2. Adapter status proves hardware integration is isolated and honest about commissioning state.
3. The scenario run proves fault injection is data-driven.
4. The benchmark proves timing, watchdog, and motion metrics are exported.
5. The capture manifest proves future bench runs have predefined acceptance criteria.

The best interview angle is not "it simulates a motor." The stronger angle is: the project defines control behavior, failure behavior, evidence artifacts, and the hardware boundary clearly enough that a reviewer can challenge each layer.
