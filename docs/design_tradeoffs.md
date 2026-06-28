# Design Tradeoff Note: FreeRTOS vs PLC Cycle Logic vs Linux Supervisory Control

## FreeRTOS-Style MCU Control

Best for deterministic low-level drive control close to I/O. The controller loop maps naturally to a high-priority periodic task fed by a hardware timer. It gives tight control over jitter sources but requires careful memory, ISR, and watchdog design.

## PLC Cycle Logic

Best for plant-level sequence control, interlocks, operator workflows, and maintainability by controls teams. PLC scan-cycle logic is familiar in industrial environments, but high-performance multi-axis control is usually delegated to motion modules or drives.

## Linux/PREEMPT_RT Supervisory Control

Best for diagnostics, orchestration, logging, analytics, and integration with fieldbus masters. PREEMPT_RT can run cyclic tasks with good determinism, but hardware selection and kernel configuration matter. It is a strong fit for the host simulation layer and future EtherCAT master prototype.

## Project Choice

This project keeps the motion-control core deterministic and portable, while separating bus and host-supervisory concerns. That lets the same validation scenarios run before hardware exists and later remain useful during commissioning.
