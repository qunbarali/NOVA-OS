# NOVA OS Alpha 0.4 — Per-task stack isolation

## Delivered in source

- Every task owns a fixed, 8-byte-aligned 512-byte stack.
- Each stack has a four-word guard region and a fill-pattern high-water measurement.
- Task callbacks execute on their own stack through an architecture-specific call-on-stack routine.
- Separate ARM assembly ports are selected for RP2040 Cortex-M0+ and RP2350 Cortex-M33.
- A guard violation marks the task FAULTED and prevents further callback dispatch.
- The shell reports configured stack size and measured high-water usage.
- Pico SDK compatibility is restored by enabling GNU C extensions for SDK runtime sources; NOVA's C standard remains C11.

## Important execution-model boundary

This milestone isolates each callback invocation on its task-owned stack, then returns to the scheduler stack when the callback returns. It does **not** preserve a suspended C call chain between scheduler polls. Tasks remain cooperative state-machine callbacks and must return promptly. This is not yet a resumable thread context switch, preemptive scheduler, process boundary, or memory-protection boundary.

The 512-byte stack size is a development default, not a universal fit for arbitrary callbacks. Stack high-water data is approximate and guard words detect only overflow that reaches the guard. They cannot guarantee recovery from arbitrary memory corruption.

## Validation status

The checked-in host scheduler test uses a host-only stack-switch stub and validates scheduler behavior and stack metadata. The two assembly ports still require the manual GitHub Actions cross-build and physical RP2040 and RP2350 board tests. No hardware validation is implied by this source milestone.

## Follow-on work (Alpha 0.5 and later)

- Resumable cooperative context switching on separate task stacks was added in Alpha 0.5. See [Alpha 0.5 notes](ALPHA_0_5.md).
- Architecture exception frames and safe fault entry on Cortex-M0+ and Cortex-M33 remain outstanding.
- Stronger stack guard strategy and stack-overflow crash records remain outstanding.
- Cross-core synchronization and explicit ownership before core 1 accesses kernel services remain outstanding.
- Preemption must wait until context preservation, interrupt nesting, timing and fault recovery are proven on hardware.
