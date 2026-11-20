# NOVA OS Alpha 0.2 — Kernel services

## Delivered source components
- Fixed-slot cooperative task scheduler (8 task slots).
- Task create, suspend, resume, terminate, sleep and state snapshot APIs.
- Wrap-safe millisecond deadline comparisons.
- Fixed block memory pool (8 blocks x 32 bytes).
- Bounded message queue (8 messages x 32 bytes).
- Non-blocking mutex primitive and event flags.
- Kernel poll integration into the USB CDC shell loop.
- Interactive LED demonstration task (`task demo`, `task stop`).
- Updated shell diagnostics and source-contract tests.

## Scheduling contract
Task entry functions are state-machine steps, not long-running blocking functions. Each call must do bounded work and return. Returning `NOVA_TASK_AGAIN` leaves the task ready for a future poll; returning `NOVA_TASK_EXIT` releases its slot. `nova_task_sleep_current(ms)` marks the current task sleeping until the deadline. The scheduler uses wrap-safe unsigned millisecond arithmetic.

## Explicit limitations
- This is cooperative polling on core 0, not preemptive multitasking.
- Tasks share the core-0 call stack. Independent execution stacks and context switching are NOT implemented in Alpha 0.2.
- Core 1 remains a diagnostic heartbeat loop and is not a second scheduler.
- The current mutex, event and queue primitives are single-core, non-blocking building blocks; they are not SMP-safe and do not provide blocking wait queues or priority inheritance.
- The fixed pool is a small kernel allocation primitive, not a general heap.
- No user/kernel privilege separation, process isolation or memory protection is provided.
- No hardware-in-loop or physical Pico/Pico 2 test is claimed by this source commit.

## Suggested serial test
```text
version
tasks
task demo
tasks
memory
task stop
tasks
```

Observe the LED pulse after starting the demo. Stop it and verify the LED turns off and its task slot is released.

## Next engineering step
Alpha 0.3 should add architecture-specific context switching only after designing and testing task stack frames for RP2040 Cortex-M0+ and RP2350 Cortex-M33. Interrupt priorities, critical sections, stack guards and crash recovery must be specified before enabling preemption.
