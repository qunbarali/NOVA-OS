# NOVA OS — architecture baseline

## Mission
NOVA is an independently designed embedded operating system, not a Linux, Android, Windows or iOS derivative. It begins on RP2040/RP2350 and is intended to scale to more capable processors through explicit architecture ports.

## Architecture principles
- Hardware abstraction separates kernel policy from silicon-specific details.
- No dynamic allocation is required during early boot.
- Interrupt handlers remain deterministic and short.
- Kernel services communicate through explicit bounded queues/events rather than unbounded shared state.
- Device access will be capability-oriented when application isolation exists.
- Interfaces are versioned; optional hardware is exposed through capability discovery.
- Recovery must remain possible if a NOVA image is damaged.

## Current Alpha 0.6 execution model
- Core 0 initializes the board/kernel and owns the USB CDC command shell.
- Core 0 runs an eight-slot cooperative state-machine scheduler from the shell's polling loop. Ready tasks are dispatched by numeric priority (lower value first); equal-priority tasks use rotating scan order.
- A repeating 1 ms hardware timer callback increments an atomic system tick counter.
- Core 1 runs a diagnostic heartbeat service; it is not a second scheduler.
- Task callbacks run on persistent per-task stacks. They may yield or sleep and resume at the instruction following that call; returning NOVA_TASK_AGAIN rebuilds the stack and restarts the callback, while NOVA_TASK_EXIT terminates it.
- Invalid task callback return values move that task to a FAULTED state; the slot remains inspectable and must be explicitly terminated.
- Each task owns a fixed 512-byte, 8-byte-aligned stack. Architecture-specific context-switch routines save and restore task registers and PSP state on Cortex-M0+ and Cortex-M33. Task code runs on PSP, while a dedicated 1 KiB MSP stack serves exception handlers during task execution.
- Four guard words detect writes into the low-address stack boundary; a low-address-first fill-pattern scan reports the deepest touched stack address as a high-water estimate. Guard corruption marks the task FAULTED.
- Task call chains persist across cooperative yields and sleeps. Scheduling remains non-preemptive: a task that does not yield, sleep, or return can monopolize core 0. Tasks share one address space and privilege level; process isolation is not implemented.
- A fixed memory pool provides eight 32-byte blocks; it is not a general-purpose heap.
- IPC currently provides an eight-entry, 32-byte-per-message queue, task-owner-tracked non-blocking mutexes and event flags.
- Core-local interrupt save/restore critical sections are available for short core-0 sections. They do not disable interrupts on the other core and are not an SMP lock.
- Queue/mutex/event primitives are single-core only and are not safe for concurrent core-1 use.
- The shell has a bounded line buffer and rejects overlong commands.
- The memory command reports physical SRAM capacity and fixed-pool availability, not total free SRAM.
- This is an embedded kernel foundation, not yet a protected microkernel with process isolation.

## Planned kernel services
1. Exception frames, fault entry and stronger stack protection
2. Interrupt-safe critical sections and priority-aware synchronization
3. General memory allocator and per-task resource accounting
4. Blocking IPC, wait queues and event subscriptions
5. Device manager and HAL
6. Block device interface and filesystem
7. USB device/host services
8. Native executable loader/runtime
9. Shell, display manager and GUI
10. Process isolation and capability enforcement

## Processor strategy
- RP2040 uses dual Cortex-M0+ cores and 264 KiB SRAM.
- RP2350 supports dual Cortex-M33 or Hazard3 RISC-V cores and 520 KiB SRAM.
- Current firmware targets RP2040 and RP2350 Cortex-M33 to maximize shared C implementation.
- RP2350 Hazard3 is a separate architecture port; ARM assembly and processor-specific code must not leak into common kernel interfaces.
- Preemptive context switching must be implemented and validated separately for Cortex-M0+ and Cortex-M33.

## Storage and boot semantics
- Internal QSPI flash: initial firmware and system image.
- SD card over SPI: preferred first removable block-storage target.
- USB mass-storage host: later, after host-stack, connector and power-path validation.
- USB UF2 recovery/install: board ROM bootloader path.
- USB-thumb-drive boot: separate future boot-source capability, not implied by UF2 installation.

## Security and reliability
- Do not write RP2350 OTP during development; OTP writes are irreversible.
- Do not modify boot metadata or bootloader regions during Alpha development.
- Future firmware slots need image validation, rollback metadata and power-failure-safe activation.
- Treat USB input and filesystem metadata as untrusted.
- Add privilege boundaries only when context switching, stack ownership and interrupt rules are validated.

## SDK boundary
The Raspberry Pi Pico SDK is currently used as a hardware support and build layer for startup, clocks, GPIO, multicore, USB CDC, timer and watchdog access. NOVA's kernel policy, shell, task model, IPC APIs and future application ABI/filesystem are independently authored. Long-term independence requires replacing SDK-dependent services incrementally with NOVA HAL implementations; the current firmware is not yet SDK-independent.
