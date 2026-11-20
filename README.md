# NOVA OS — Kernel Alpha 0.3.0

NOVA is an independently designed embedded operating system project. Alpha 0.3 adds priority-aware cooperative dispatch, task fault containment and core-local interrupt critical sections for Raspberry Pi Pico (RP2040) and Raspberry Pi Pico 2 (RP2350 Cortex-M33 target).

## Alpha 0.3 features
- Eight-slot cooperative state-machine scheduler with priority-ordered dispatch and round-robin tie-breaking
- Task creation, suspension, resumption, termination and sleep
- Task state, priority, dispatch and fault diagnostics
- Invalid task callback results are isolated as FAULTED slots
- Core-local interrupt save/restore critical-section API for short core-0 sections
- 1 ms hardware timer interrupt with atomic system tick counter
- Fixed memory pool: eight 32-byte blocks
- Bounded IPC message queues: eight 32-byte messages with variable-length payloads
- Task-owner-tracked, non-blocking mutex and event-flag primitives
- Scheduler polling integrated into the USB CDC shell
- LED demo task: `task demo` / `task stop`
- Separate Pico and Pico 2 build targets

## Windows prerequisites
Install Git, CMake, Ninja, Python 3, ARM GNU Toolchain (`arm-none-eabi-gcc`) and the Raspberry Pi Pico SDK with its required submodules. Set `PICO_SDK_PATH` to the SDK directory.

## Build both targets

```powershell
.\scripts\build-all.ps1
```

Expected outputs: `build-pico\nova_os.uf2` and `build-pico2\nova_os.uf2`.

## Install and test
Put the board into BOOTSEL mode, copy the matching UF2 to its `RPI-RP2` drive, then open the USB CDC COM port. Try:

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

## Manual GitHub firmware builds
Open **Actions → NOVA OS firmware build → Run workflow**. The workflow is manual-only and builds both RP2040 (`pico`) and RP2350 Cortex-M33 (`pico2`) targets using the pinned Pico SDK 2.3.1. It runs source-contract and native scheduler tests, then publishes separate UF2/ELF artifacts for 14 days. No push or pull-request trigger is configured.

## Tests
Run source-contract checks:

```powershell
python -m unittest discover -s tests -v
```

These checks are not a substitute for compiling both firmware targets or testing on physical boards.

## Important architecture limitations
Alpha 0.2 uses cooperative state-machine tasks on core 0. Tasks share the core-0 stack; independent task stacks and preemptive context switching are not implemented. Core 1 remains a diagnostic heartbeat. Queue, mutex and event primitives are bounded, non-blocking and single-core only. No process isolation or general-purpose heap exists yet.

See [Alpha 0.3 notes](docs/ALPHA_0_3.md) and [Alpha 0.2 design notes](docs/ALPHA_0_2.md), [architecture](docs/ARCHITECTURE.md) and [Phase 1 hardware test plan](docs/PHASE1_TEST_PLAN.md).
