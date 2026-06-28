# Hardware Target Profiles

The records in [`../config/hardware_targets`](../config/hardware_targets) are vendor-neutral planning profiles. They are not purchase recommendations. Their purpose is to define what a future hardware target must prove before the project claims real bench validation.

## Current Profiles

| Profile | Purpose |
| --- | --- |
| `simulated_loopback_target` | Baseline software-validation target used for CI and portfolio evidence. |
| `generic_ethercat_cia402_servo` | Future EtherCAT CiA 402 servo/drive planning record. |
| `generic_canopen_cia402_servo` | Future CANopen CiA 402 servo/drive planning record. |

Each profile records required interfaces, expected CiA 402 objects, capture profiles, pre-purchase questions, commissioning risks, and evidence required.

## Generated Manifest

The generated manifest files are:

- [`../reports/sample_hardware_target_manifest.json`](../reports/sample_hardware_target_manifest.json)
- [`../reports/sample_hardware_target_manifest.md`](../reports/sample_hardware_target_manifest.md)

## Boundary

These profiles deliberately avoid vendor-specific purchasing claims. Real hardware selection must still consider availability, local support, safety documentation, drive manuals, interface compatibility, and bench constraints at the time of purchase.
