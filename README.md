# NOVA OS — Kernel Alpha 1.2.0

NOVA is an independently designed embedded operating system project for Raspberry Pi Pico (RP2040) and Raspberry Pi Pico 2 (RP2350 Cortex-M33 target). Alpha 1.2 brings together the device registry, blocking IPC, cross-core shared-service locking, task stack protection and HardFault diagnostics built across Alpha 0.5–1.2.

## Alpha 1.2 features
- Correct per-task stack high-water measurement for downward-growing Cortex-M stacks
- Initial saved context frame included in stack usage baseline
- Four-word stack guard checks retained; guard corruption faults the task
- Stack high-water budget is capped at 480 of 512 bytes; tasks crossing the budget are quarantined
- Eight-slot resumable cooperative scheduler with priority-ordered dispatch and round-robin tie-breaking
- Task creation, suspension, resumption, termination, cooperative yield, resumable sleep and blocked wait states
- Timeout-aware blocking queue send/receive and event-flag waits
- Static device registry with typed classes, capability flags and driver read/write/control operations
- Onboard LED exposed as the `led0` GPIO device; inspect with `devices`
- Per-task saved stack pointers and architecture-specific Cortex-M0+ / Cortex-M33 context switches
- Task state, priority and dispatch diagnostics
- Bounded eight-entry task fault journal with sequence, tick, task identity and reason; inspect with `faults`
- Minimal Cortex-M HardFault frame capture retained in watchdog scratch registers; inspect with `crash` after watchdog restart
- Scheduler-detected stack-guard, invalid-result and invalid-return-state fault records
- Invalid task callback results are isolated as FAULTED slots
- Core-local interrupt save/restore critical-section API for short core-0 sections
- RP hardware-spinlock service protecting shared IPC primitives and fixed memory-pool metadata across cores
- 1 ms hardware timer interrupt with atomic system tick counter
- Fixed memory pool: eight 32-byte blocks
- Bounded IPC message queues: eight 32-byte messages with variable-length payloads
- Task-owner-tracked, non-blocking mutex and event-flag primitives
- Scheduler polling integrated into the USB CDC shell
- LED demo task: task demo / task stop
- Separate Pico and Pico 2 build targets

## Windows prerequisites
Install Git, CMake, Ninja, Python 3, ARM GNU Toolchain (arm-none-eabi-gcc) and the Raspberry Pi Pico SDK with its required submodules. Set PICO_SDK_PATH to the SDK directory.

## Build both targets

```powershell
.\scripts\build-all.ps1
```

Expected outputs: build-pico\nova_os.uf2 and build-pico2\nova_os.uf2.

## Install and test
Put the board into BOOTSEL mode, copy the matching UF2 to its RPI-RP2 drive, then open the USB CDC COM port. Try:

```text
version
system
tasks
task demo
tasks
memory
devices
device led0 on
device led0 off
task stop
tasks
```

## Manual GitHub firmware builds
Open Actions → NOVA OS firmware build → Run workflow. The workflow is manual-only and builds both RP2040 (pico) and RP2350 Cortex-M33 (pico2) targets using the pinned Pico SDK 2.3.1. It runs source-contract, native scheduler, IPC and device-manager tests, then publishes separate UF2/ELF artifacts for 14 days. No push or pull-request trigger is configured.

## Tests
Run source-contract checks:

```powershell
python -m unittest discover -s tests -v
```

These checks are not a substitute for compiling both firmware targets or testing on physical boards.

## Important architecture limitations
Alpha 1.2 retains cooperative, resumable tasks on independent 512-byte stacks. Scheduling is not preemptive: tasks must yield, sleep or return. Core 1 remains a diagnostic heartbeat. Queue, mutex, event, device registry, onboard LED state and fixed-pool metadata are protected by an RP hardware spinlock; task scheduling remains core-0-only. Stack guards and high-water reporting are diagnostics, not MPU protection. No process isolation or general-purpose heap exists yet.

See [Alpha 1.2 notes](docs/ALPHA_1_2.md), [Alpha 1.1 notes](docs/ALPHA_1_1.md), [Alpha 1.0 notes](docs/ALPHA_1_0.md), [Alpha 0.9 notes](docs/ALPHA_0_9.md), [Alpha 0.8 notes](docs/ALPHA_0_8.md), [Alpha 0.7 notes](docs/ALPHA_0_7.md), [Alpha 0.6 notes](docs/ALPHA_0_6.md), [Alpha 0.5 notes](docs/ALPHA_0_5.md), [Alpha 0.4 notes](docs/ALPHA_0_4.md), [Alpha 0.3 notes](docs/ALPHA_0_3.md), [Alpha 0.2 design notes](docs/ALPHA_0_2.md), [architecture](docs/ARCHITECTURE.md) and [Phase 1 hardware test plan](docs/PHASE1_TEST_PLAN.md).
