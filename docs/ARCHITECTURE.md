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

## Current Alpha 1.5 execution model
- Core 0 initializes the board/kernel and owns the TinyUSB CDC command shell; the composite USB device also exposes an MSC interface.
- Board startup selects ST7735 or ILI9341, initializes RGB565 drawing and a batched 5x7 ASCII font renderer. Panel dimensions, offsets, orientation and color order still require validation.
- The four-key input HAL samples active-low Up/Down/Select/Back GPIOs with pull-ups and 25 ms debounce. The graphical launcher consumes latched events from the main loop.
- Board startup initializes the selected panel, keypad and SD SPI card. A detected card is registered as sd0 and bound to FatFs and USB MSC. The suggested GPIO map must be matched to the carrier wiring.
- The SD layer supports SPI initialization, CSD capacity detection, 512-byte sector reads/writes, bounds checks and a 32 GiB ceiling. FatFs R0.16 provides FAT12/16/32 and LFN. Filesystem calls are serialized; the kernel filesystem and USB MSC use an exclusive storage-owner gate.
- Core 0 runs an eight-slot cooperative state-machine scheduler from the shell's polling loop. Ready tasks are dispatched by numeric priority (lower value first); equal-priority tasks use rotating scan order.
- A repeating 1 ms hardware timer callback increments an atomic system tick counter.
- TinyUSB exposes CDC and MSC in device mode. MSC directly reads/writes SD blocks and refuses readiness while NOVA owns the mounted FAT volume. Eject keeps the card unavailable until USB disconnect.
- The launcher browses the SD root and NOVA/APPS. Firmware validates .nova package headers, board target, ABI, size and SHA-256 without loading the whole file into RAM. The ABI v1 service table is defined; dynamic ELF loading and application execution are not implemented.
- Optional battery ADC telemetry averages 16 samples and is disabled until the actual divider ratio and ADC GPIO are configured. It is a voltage estimate, not a charger or fuel gauge.
- Core 1 runs a diagnostic heartbeat service; it is not a second scheduler.
- Task callbacks run on persistent per-task stacks. They may yield or sleep and resume at the instruction following that call; returning NOVA_TASK_AGAIN rebuilds the stack and restarts the callback, while NOVA_TASK_EXIT terminates it.
- Scheduler-detected task faults move the task to a FAULTED state; the slot remains inspectable and must be explicitly terminated. A bounded eight-entry journal records sequence, scheduler tick, task ID/name and reason; the `faults` command displays retained records. Cortex-M HardFault entry stubs capture a compact basic exception frame into watchdog scratch registers, request watchdog reboot and expose the retained record through `crash`; this is not a complete crash dump or exception recovery system.
- Each task owns a fixed 512-byte, 8-byte-aligned stack. Architecture-specific context-switch routines save and restore task registers and PSP state on Cortex-M0+ and Cortex-M33. Task code runs on PSP, while a dedicated 1 KiB MSP stack serves exception handlers during task execution.
- Four guard words detect writes into the low-address stack boundary; a low-address-first fill-pattern scan reports the deepest touched stack address as a high-water estimate. Guard corruption marks the task FAULTED.
- Task call chains persist across cooperative yields and sleeps. Scheduling remains non-preemptive: a task that does not yield, sleep, or return can monopolize core 0. Tasks share one address space and privilege level; process isolation is not implemented.
- A fixed memory pool provides eight 32-byte blocks; it is not a general-purpose heap. Pool metadata is protected by the shared RP hardware spinlock.
- A static device manager registers up to eight named devices with class, capability flags, driver operations and context. The board layer registers the onboard LED as `led0`; shell inventory is available through `devices`. This registry is not yet a security capability system and has no hot-unplug or driver lifetime management.
- IPC provides an eight-entry, 32-byte-per-message queue, task-owner-tracked non-blocking mutexes and event flags. Tasks can block on queue send/receive and event waits with optional millisecond timeouts; the scheduler resumes them on wake requests or timeout.
- Core-local interrupt save/restore critical sections are available for short core-0 sections. They do not disable interrupts on the other core and are not an SMP lock.
- Queue, mutex, event, device-registry and fixed-pool metadata operations use a shared RP hardware spinlock. Wake requests use an atomic pending bit consumed by the core-0 scheduler. The task scheduler and task-control structures remain core-0-only; core 1 must not create, suspend, block, or inspect tasks. Blocking queue/event calls require a running core-0 task. The lock APIs are not ISR-reentrant.
- The shell has a bounded line buffer and rejects overlong commands.
- The memory command reports physical SRAM capacity and fixed-pool availability, not total free SRAM.
- This is an embedded kernel foundation, not yet a protected microkernel with process isolation.

## Planned kernel services
1. Complete exception diagnostics, configurable stack budgets and recovery policies
2. Interrupt-safe critical sections and priority-aware synchronization
3. General memory allocator and per-task resource accounting
4. General memory allocator and per-task resource accounting
5. Native executable loader/runtime with target-specific memory maps
6. USB host mode and accessory drivers
7. Firmware update/recovery protocol
8. Process isolation and capability enforcement

## Processor strategy
- RP2040 uses dual Cortex-M0+ cores and 264 KiB SRAM.
- RP2350 supports dual Cortex-M33 or Hazard3 RISC-V cores and 520 KiB SRAM.
- Current firmware targets RP2040 and RP2350 Cortex-M33 to maximize shared C implementation.
- RP2350 Hazard3 is a separate architecture port; ARM assembly and processor-specific code must not leak into common kernel interfaces.
- Preemptive context switching must be implemented and validated separately for Cortex-M0+ and Cortex-M33.

## Storage and boot semantics
- Internal QSPI flash: initial firmware and system image.
- SD card over SPI: preferred first removable block-storage target.
- USB mass-storage device: exposes the SD card to a connected host under exclusive ownership.
- USB host mode: future, after host-stack, connector and power-path validation.
- USB UF2 recovery/install: board ROM bootloader path.
- USB-thumb-drive boot: separate future boot-source capability, not implied by UF2 installation.

## Security and reliability
- Do not write RP2350 OTP during development; OTP writes are irreversible.
- Do not modify boot metadata or bootloader regions during Alpha development.
- Future firmware slots need image validation, rollback metadata and power-failure-safe activation.
- Treat USB input and filesystem metadata as untrusted.
- Add privilege boundaries only when context switching, stack ownership and interrupt rules are validated.

## SDK boundary
The Raspberry Pi Pico SDK is currently used as a hardware support and build layer for startup, clocks, GPIO, multicore, TinyUSB device mode, timer, ADC and watchdog access. FatFs is vendored as the FAT implementation. NOVA's kernel policy, shell, task model, IPC APIs and future application ABI/filesystem are independently authored. Long-term independence requires replacing SDK-dependent services incrementally with NOVA HAL implementations; the current firmware is not yet SDK-independent.
