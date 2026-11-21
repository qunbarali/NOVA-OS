# NOVA OS — Kernel Alpha 1.3.0

NOVA is an independently designed embedded operating system for Raspberry Pi Pico (RP2040) and Raspberry Pi Pico 2 (RP2350 Cortex-M33 target). Alpha 1.3 adds the first optional display-controller and four-button input foundations on top of the Alpha 1.2 kernel.

## Alpha 1.3 additions
- Common RGB565 display API with selectable ST7735 and ILI9341 controllers
- Configurable panel dimensions and X/Y offsets for common display-module variants
- SPI display initialization, screen fill, rectangle and pixel primitives
- Four-button GPIO input API: Up, Down, Select and Back
- Active-low buttons with internal pull-ups, stable-state tracking and 25 ms debounce
- Latched press events consumed by the application/UI polling loop
- Source-contract checks for both display controller paths and the keypad API

## Alpha 1.2 kernel capabilities
- Eight-slot cooperative scheduler with priority dispatch and round-robin tie-breaking
- Persistent task stacks, stack guards, high-water measurement and task fault journal
- Timeout-aware queue and event blocking APIs
- Static device registry and onboard LED device
- RP hardware-spinlock protection for shared IPC and memory-pool metadata
- Cortex-M0+ and Cortex-M33 context-switch and HardFault entry ports
- USB CDC shell, timer ticks and board diagnostics

## Display modules
Supported controller families:
- ST7735: configure the actual module's visible width/height (commonly 128x160) and offsets. Variants may use different offsets, color order and initialization settings.
- ILI9341: configure the actual module's visible width/height (commonly 240x320) and offsets.

The display driver currently uses SPI and a dedicated chip-select, D/C and reset signal. Backlight is optional. The current implementation is a controller foundation; module-specific orientation, RGB/BGR order and clone-specific initialization still require hardware validation.

## Proposed prototype wiring
This is a suggested pin assignment, not a board-wide fixed standard:
- SPI0: GP18 SCK, GP19 MOSI, GP16 MISO
- Display: GP13 CS, GP12 D/C, GP11 RESET, GP10 optional backlight
- SD card: GP17 CS (future driver; shared SPI arbitration is not yet implemented)
- Four active-low buttons: GP2 Up, GP3 Down, GP4 Select, GP5 Back

Do not connect a module until its pin labels and logic-voltage requirements are checked. Avoid powering a 5 V-only module signal directly from a Pico GPIO.

## Current limitations
- Display and SD do not yet share an SPI bus manager; SD/FAT32 is not implemented.
- The keypad driver is available but is not yet enabled in board startup or connected to a graphical launcher.
- There is no display text/font renderer or application launcher yet.
- The Windows NOVA IDE is specified but not implemented.
- No NOVA application ABI, executable loader or process isolation exists. Arbitrary ELF/BIN execution is not supported.
- UF2 remains a firmware flashing/recovery image, not a normal SD-launched application.
- Display controller support has not yet been compiled by the user's manual workflow or validated on physical panels.

## Windows prerequisites
Install Git, CMake, Ninja, Python 3, ARM GNU Toolchain (arm-none-eabi-gcc) and Raspberry Pi Pico SDK with required submodules. Set PICO_SDK_PATH.

## Build both targets
```powershell
.\scripts\build-all.ps1
```
Expected outputs: build-pico\\nova_os.uf2 and build-pico2\\nova_os.uf2.

## Source-contract tests
```powershell
python -m unittest discover -s tests -v
```
These tests do not replace cross-compiling both boards or physical hardware validation.

## Manual GitHub firmware builds
Open Actions → NOVA OS firmware build → Run workflow. The workflow is manual-only, targets RP2040 and RP2350 Cortex-M33 using Pico SDK 2.3.1, and publishes firmware artifacts. No push or pull-request trigger is configured.

See [Alpha 1.3 notes](docs/ALPHA_1_3.md), [display and application platform](docs/DISPLAY_AND_APPLICATION_PLATFORM.md), [Windows IDE specification](docs/NOVA_WINDOWS_IDE_SPEC.md), [architecture](docs/ARCHITECTURE.md), and [hardware test plan](docs/PHASE1_TEST_PLAN.md).
