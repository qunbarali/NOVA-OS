# NOVA OS — architecture baseline

## Mission
NOVA is an independently designed operating system, not a Linux, Android, Windows or iOS derivative. It begins on RP2040/RP2350 and is intended to scale to more capable processors through explicit architecture ports.

## Architecture principles
- Hardware abstraction separates kernel policy from silicon-specific details.
- No dynamic allocation is required during early boot.
- Interrupt handlers remain deterministic and short.
- Kernel services communicate through explicit queues/events rather than shared unbounded state.
- Device access will be capability-oriented when application isolation exists.
- Interfaces are versioned; optional hardware is exposed through capability discovery.
- Recovery must remain possible if a NOVA image is damaged.

## Current Alpha 0.1.1 execution model
- Core 0 initializes the board/kernel and owns the USB CDC command shell.
- Core 1 runs a diagnostic heartbeat service.
- Heartbeat and readiness diagnostics use compiler atomic operations.
- The shell has a bounded line buffer and rejects overlong commands.
- `memory` reports physical SRAM capacity only; it does not claim to know free memory.
- This is a bring-up kernel, not yet a microkernel with process isolation or a scheduler.

## Planned kernel services
1. Boot and board identification
2. Interrupt/event subsystem
3. Bounded cooperative scheduler and task lifecycle
4. Per-task stacks and stack guards
5. Fixed-size memory pools and heap
6. IPC queues and event channels
7. Device manager and HAL
8. Block device interface and filesystem
9. USB device/host services
10. Native executable loader/runtime
11. Shell, display manager and GUI

## Processor strategy
- RP2040 uses dual Cortex-M0+ cores and 264 KiB SRAM.
- RP2350 supports dual Cortex-M33 or Hazard3 RISC-V cores and 520 KiB SRAM.
- Phase 1 targets RP2040 and RP2350 Cortex-M33 to maximize shared C implementation.
- RP2350 Hazard3 is a separate architecture port; ARM assembly and processor-specific code must not leak into common kernel interfaces.

## Storage and boot semantics
- Internal QSPI flash: initial firmware and system image.
- SD card over SPI: preferred first removable block-storage target.
- USB mass-storage host: later, after host-stack, connector and power-path validation.
- USB UF2 recovery/install: board ROM bootloader path.
- USB-thumb-drive boot: separate future boot-source capability, not implied by UF2 installation.

## Security and reliability
- Do not write RP2350 OTP during development; OTP writes are irreversible.
- Do not modify boot metadata or bootloader regions in Alpha 0.1.
- Future firmware slots need image validation, rollback metadata and power-failure-safe activation.
- Treat USB input and filesystem metadata as untrusted.
- Add privilege boundaries only when context switching, stack ownership and interrupt rules are validated.

## SDK boundary
The Raspberry Pi Pico SDK is currently used as a hardware support and build layer for startup, clocks, GPIO, multicore, USB CDC and watchdog access. NOVA's kernel policy, shell, task model, application ABI and filesystem are independently authored. Long-term independence requires replacing SDK-dependent services incrementally with NOVA HAL implementations; the current firmware is not yet SDK-independent.
