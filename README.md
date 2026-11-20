# NOVA OS — Kernel Alpha 0.1.1

NOVA is an independently designed embedded operating system project. This milestone hardens the first hardware bring-up shell for Raspberry Pi Pico (RP2040) and Raspberry Pi Pico 2 (RP2350, Cortex-M33 target).

## Implemented in Alpha 0.1.1
- Shared C11 codebase with separate Pico/Pico 2 build configurations
- Raspberry Pi Pico SDK used as the hardware support layer, not as an OS
- Onboard LED HAL
- Core-1 heartbeat service with atomic diagnostics
- USB CDC command shell
- Board/chip identification and unique board ID reporting
- SRAM capacity reporting (total physical capacity, not free memory)
- Bounded command input with overflow rejection and backspace handling
- Controlled watchdog reboot command
- Windows PowerShell dual-target build and UF2 install guidance
- Source-contract tests that can run without attached hardware

## Windows prerequisites
Install Git, CMake, Ninja, Python 3, ARM GNU Toolchain (`arm-none-eabi-gcc`) and the Raspberry Pi Pico SDK with its required submodules.

Set `PICO_SDK_PATH` to the Pico SDK directory. See the official [Pico setup guide](https://github.com/raspberrypi/pico-setup) and [C/C++ SDK documentation](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html).

## Build both boards (PowerShell)

```powershell
$env:PICO_SDK_PATH = 'C:\pico\pico-sdk'
.\scripts\build-all.ps1
```

Expected outputs: `build-pico\nova_os.uf2` and `build-pico2\nova_os.uf2`.

## Install firmware
1. Disconnect the board from USB.
2. Hold BOOTSEL while connecting it to the computer.
3. Wait for the `RPI-RP2` drive to appear.
4. Copy the matching UF2 file to that drive.
5. The board reboots into NOVA.

This is ROM UF2 firmware installation/recovery. It is not booting NOVA from a USB thumb drive.

## Serial console
Open the board's USB CDC COM port at 115200 baud, then press Enter. Commands:

```text
help
version
system
cpu
memory
tasks
uptime
led on
led off
reboot
```

## Source tests

```powershell
python -m unittest discover -s tests -v
```

These tests check source contracts only. They do not replace firmware compilation, hardware-in-loop testing or physical board validation.

## Current limitations
- Core 1 heartbeat is a bring-up service, not a general-purpose scheduler.
- No user-mode isolation, syscall boundary or preemptive context switching yet.
- No dynamic memory allocator or memory accounting yet.
- No filesystem, SD-card or USB mass-storage host implementation yet.
- No native executable loader, application permissions or GUI yet.
- No physical Pico/Pico 2 test has been performed by this build environment.

See `docs/ARCHITECTURE.md` and `docs/PHASE1_TEST_PLAN.md`.