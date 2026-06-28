# Adapter Replay Harness

The replay harness feeds recorded single-axis CiA 402 process-image frames through the same `IBusBackend`, wire-image scaling, and `AdapterExchangeMonitor` contracts intended for real EtherCAT and CANopen adapters.

It is an integration aid, not hardware validation. A passing replay proves that captured exchange semantics match the software contract; it does not prove electrical behavior, bus timing, drive configuration, or physical motion.

## CSV Contract

Replay files live under [`../../examples/replay`](../../examples/replay) and use a fixed, unquoted CSV header:

```text
cycle,rx_update,online,heartbeat_ok,invalid_response,statusword,actual_position_counts,actual_velocity_counts,drive_fault,expected_controlword,expected_target_position_counts,expected_target_velocity_counts,expected_contract_fault
```

Each frame declares:

- bus and heartbeat state;
- whether fresh receive process data arrived;
- received statusword, actual position, actual velocity, and drive fault;
- the outgoing controlword and target values the controller must provide;
- the adapter-contract fault expected at that cycle.

Cycles must be strictly increasing. Counts are converted with the configured counts-per-unit scale. The backend rejects an unexpected cycle, axis count, controlword, target position, or target velocity as an invalid response.

## Run

```powershell
build/emrtc_cli run replay examples/replay/nominal_exchange.csv
build/emrtc_cli run replay examples/replay/fault_recovery_exchange.csv
```

The nominal trace verifies fresh process data and final feedback. The fault/recovery trace verifies:

- stale receive data reaches the configured limit;
- a fresh frame restores the stale-data monitor;
- bus offline is distinguished from heartbeat loss;
- invalid response is surfaced through the shared contract;
- fresh receive data is accepted after each fault condition.

## Converting A Bench Capture

1. Export one row per control cycle from the native fieldbus capture.
2. Convert mapped PDO/object values to the fixed columns above.
3. Preserve raw counts and record the counts-per-unit value used for replay.
4. Mark `rx_update=false` when no fresh receive image was delivered.
5. Set expected contract faults from the runbook acceptance criteria.
6. Archive the original capture and conversion notes with the run manifest.
7. Run replay in CI before accepting changes to adapter mapping or fault handling.

Replay files derived from hardware remain evidence derivatives. The original bus capture, mapping dump, drive parameters, commit hash, and archive checksums remain authoritative.
