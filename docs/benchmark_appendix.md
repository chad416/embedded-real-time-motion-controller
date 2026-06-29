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
- Profile-position target, final position, overshoot, settling time, steady-state error, and max following error.
- Two-axis final position, RMS path error, max path error, and settling time.
- Bus TX/RX packets.
- Packet-loss count.
- Dropout, invalid-response, heartbeat-timeout counters.
- Fault detection latency, operation re-enable cycles, baseline/post-recovery settling, settling ratio, and post-recovery final error.

## Why Synthetic Timing Is Used

The deterministic benchmark validates controller behavior and repeatable evidence without host scheduler noise. The separate wall-clock probe measures absolute-deadline wakeups and host capabilities. PREEMPT_RT or MCU timer acceptance measurements must still come from the selected physical target.
