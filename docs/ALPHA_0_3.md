# NOVA OS Alpha 0.3 — Scheduler reliability foundations

## Delivered in source

- Priority-ordered cooperative dispatch (lower numeric value is higher priority).
- Rotating scan origin breaks equal-priority ties to prevent a fixed slot from always winning.
- Task callback results are validated. An unknown result moves the task to FAULTED and disables further dispatch of that callback.
- Faulted task slots remain visible in snapshots and must be explicitly terminated before reuse.
- Scheduler re-entry is rejected.
- Shell task diagnostics report priority, state, dispatch count and fault count.
- Core-local interrupt critical-section API saves and restores the caller's exact interrupt state.
- Native scheduler tests cover priority ordering and invalid callback-result containment in addition to sleep, deadline wraparound, suspend/resume and termination.

## Priority semantics

Priority is an unsigned 8-bit value. Lower values run earlier within each scheduler poll pass; the default is 128. Every eligible task is dispatched at most once per pass. Equal-priority tasks are ordered using a rotating scan origin. This is cooperative ordering, not preemption, deadline scheduling, or a guarantee that a high-priority task can interrupt a running callback.

## Critical-section semantics

nova_critical_enter() returns the interrupt state and disables interrupts on the calling core. nova_critical_exit(state) restores that exact state. These calls must be paired in the same control path and used only around short, bounded sections.

This primitive is core-local. It does not stop the other RP2040/RP2350 core and is not an SMP spinlock. Do not use it to protect data concurrently accessed by core 1. The current IPC services remain core-0-only.

## Fault containment boundary

A task callback that returns a value other than NOVA_TASK_AGAIN or NOVA_TASK_EXIT is marked FAULTED. This catches API contract violations after a callback returns. It does not catch hard faults, memory corruption, stack overflow, infinite loops, or faults that occur before the callback returns. Such failures require architecture-level exception handlers and independently owned stacks.

## Explicitly not delivered

- Cortex-M0+ or Cortex-M33 context-switch assembly.
- Independent task stacks or stack guards.
- Preemptive scheduling or time slicing.
- Interrupt-priority configuration or a cross-core lock.
- CPU exception recovery, task restart or safe recovery from arbitrary task crashes.
- Hardware validation on physical Pico/Pico 2 boards.

The scheduler still invokes callbacks on the shell's core-0 stack. The new APIs must not be interpreted as independent threads or processes.

## Required next architecture work

1. Define and test initial exception/context frames for Cortex-M0+ and Cortex-M33 separately.
2. Allocate aligned per-task stacks with guard regions and high-water tracking.
3. Implement cooperative context save/restore before considering timer-driven preemption.
4. Add fault handlers and a minimal crash record that survives a task fault without relying on corrupted task stack state.
5. Add hardware tests for context preservation, nested interrupts, stack boundaries and task switching on both boards.

Preemption must remain disabled until both architecture ports pass host-side frame-layout tests, cross-compilation, and physical-board stress tests.
