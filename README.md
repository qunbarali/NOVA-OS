# NOVA OS — Kernel Alpha 0.2.0

NOVA is an independently designed embedded operating system project. Alpha 0.2 adds bounded cooperative task scheduling and foundational kernel services for Raspberry Pi Pico (RP2040) and Raspberry Pi Pico 2 (RP2350 Cortex-M33 target).

## Alpha 0.2 features
- Eight-slot cooperative state-machine task scheduler with round-robin scan
- Task creation, suspension, resumption, termination and sleep
- Task state and dispatch diagnostics
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
tasks
task demo
tasks
memory
task stop
tasks
```

## Tests
Run source-contract checks:

```powershell
python -m unittest discover -s tests -v
```

These checks are not a substitute for compiling both firmware targets or testing on physical boards.

## Important architecture limitations
Alpha 0.2 uses cooperative state-machine tasks on core 0. Tasks share the core-0 stack; independent task stacks and preemptive context switching are not implemented. Core 1 remains a diagnostic heartbeat. Queue, mutex and event primitives are bounded, non-blocking and single-core only. No process isolation or general-purpose heap exists yet.

See [Alpha 0.2 design notes](docs/ALPHA_0_2.md), [architecture](docs/ARCHITECTURE.md) and [Phase 1 hardware test plan](docs/PHASE1_TEST_PLAN.md).
