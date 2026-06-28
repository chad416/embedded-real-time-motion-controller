# Benchmark Appendix

The benchmark uses a deterministic profile:

- 1 ms nominal cycle period.
- Bounded synthetic jitter pattern.
- Packet loss around cycle 300.
- CPU load spike around cycle 800.
- Sensor noise around cycle 1200.
- Missed cycle around cycle 1600.

## Reported Metrics

- Cycle count.
- Min/max/mean cycle time.
- Min/max/mean absolute jitter.
- Jitter histogram.
- Missed-cycle count.
- Deadline-overrun count.
- Bus TX/RX packets.
- Packet-loss count.
- Dropout, invalid-response, heartbeat-timeout counters.

## Why Synthetic Timing Is Used

The current phase validates controller behavior and evidence generation without real hardware. Real PREEMPT_RT or MCU timer measurements should replace the synthetic period source during hardware commissioning.
