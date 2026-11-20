# NOVA OS Alpha 0.2 — Kernel services

## Delivered source components
- Fixed-slot cooperative task scheduler (8 task slots).
- Task create, suspend, resume, terminate, sleep and state snapshot APIs.
- Round-robin scan with a stable scan origin; wrap-safe millisecond deadline comparisons.
- Fixed block memory pool (8 blocks x 32 bytes).
- Bounded message queue (8 messages x 32 bytes), preserving each message's actual length.
- Task-owner-tracked, non-blocking mutex primitive.
- Event flag set/get/clear primitives.
- 1 ms repeating hardware timer interrupt and atomic system tick counter.
- Kernel poll integration into the USB CDC shell loop.
- Interactive LED demonstration task (`task demo`, `task stop`).
- Updated shell diagnostics and source-contract tests.

## Scheduling contract
Task entry functions are state-machine steps, not long-running blocking functions. Each call must do bounded work and return. Returning `NOVA_TASK_AGAIN` leaves the task ready for a future poll; returning `NOVA_TASK_EXIT` releases its slot. `nova_task_sleep_current(ms)` marks the current task sleeping until the deadline. The scheduler uses wrap-safe unsigned millisecond arithmetic.

## IPC contract
Queues copy messages into bounded internal storage and retain the actual message length. Receive fails without removing a message when the caller's buffer is too small. Mutex acquisition/release is permitted only from a scheduled task and release is restricted to the owning task. Event flags are simple single-core bit flags; they do not yet provide blocking wait operations.

## Explicit limitations
- This is cooperative polling on core 0, not preemptive multitasking.
- Tasks share the core-0 call stack. Independent execution stacks and context switching are NOT implemented in Alpha 0.2.
- Core 1 remains a diagnostic heartbeat loop and is not a second scheduler.
- The queue, mutex and event primitives are single-core only; they are not SMP-safe and do not provide blocking wait queues or priority inheritance.
- The fixed pool is a small kernel allocation primitive, not a general heap.
- No user/kernel privilege separation, process isolation or memory protection is provided.
- No hardware-in-loop or physical Pico/Pico 2 test is claimed by this source commit.

## Suggested serial test
```text
version
system
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
