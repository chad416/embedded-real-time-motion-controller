# Adapter Exchange Contract

The adapter exchange contract is implemented in [`../../include/emrtc/adapter_contract.hpp`](../../include/emrtc/adapter_contract.hpp). It gives future real fieldbus adapters a shared way to reject bad mappings and stale receive data before motion is trusted.

## Mapping Validation

`validate_adapter_mapping()` compares an adapter-provided object/PDO map against the shared CiA 402 contract:

- Required fields must be present.
- Object index, sub-index, bit length, and direction must match.
- Duplicate field mappings are rejected.

Invalid mapping returns `FaultCode::InvalidDriveTransition` because the controller cannot safely interpret the drive state if the object dictionary contract is wrong.

## Exchange Monitoring

`AdapterExchangeMonitor` observes each cyclic exchange:

| Condition | Contract result |
| --- | --- |
| Bus offline | `FaultCode::BusDropout` |
| Heartbeat unhealthy | `FaultCode::HeartbeatTimeout` |
| Invalid response | `FaultCode::InvalidDriveTransition` |
| Receive packet count stale beyond limit | `FaultCode::BusDropout` |

The monitor is intentionally independent of SOEM, EtherLab, SocketCAN, or CANopenNode. Real adapters can call it after stack-specific exchange logic and before copying receive data into the controller process image.

The [Adapter Replay Harness](replay_harness.md) implements `IBusBackend` and feeds recorded wire values through this monitor. Each frame also declares the expected outgoing controlword/targets and expected contract fault.

## Current Evidence

The no-dependency C++ test runner covers:

- Valid object mapping acceptance.
- Invalid object index rejection.
- Stale receive/PDO data detection.
- Invalid response detection.
- Bus-offline detection followed by restored fresh receive data.
- Nominal recorded exchange replay.
- Stale, offline, heartbeat, invalid-response, and recovery replay.
- Rejection of an outgoing controller image that differs from the recording.

These tests do not claim hardware validation. They prove the adapter contract is explicit before a real stack is commissioned.
