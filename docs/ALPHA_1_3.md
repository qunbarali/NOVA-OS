# NOVA OS Alpha 1.3 — peripheral foundation

## Delivered
- `nova/display.h` defines a shared RGB565 display interface.
- `src/hal/display.c` contains ST7735 and ILI9341 initialization profiles and basic pixel, rectangle and fill operations.
- Panel width, height and X/Y offsets are supplied by configuration, rather than assuming all modules have identical glass geometry.
- `nova/input.h` defines four logical keys: Up, Down, Select and Back.
- `src/hal/input.c` samples active-low GPIO keys with pull-ups, applies 25 ms debounce, tracks stable state and latches press events.
- CMake includes both HAL drivers; source-contract tests check the public interfaces and controller/key paths.

## Not yet delivered
- No startup pin assignment is enabled by default because the exact display/keypad module pinout must be confirmed.
- No shared SPI bus arbitration or SD-card driver exists yet.
- No display font renderer, menu launcher or application selection UI exists yet.
- No Windows IDE executable or NOVA application runtime exists yet.
- Firmware build and physical display/button testing remain pending.

## Suggested next milestone: Alpha 1.4
1. Add a shared SPI bus manager with per-device chip-select and transaction ownership.
2. Add SD SPI initialization, SDHC block read/write and safe card detection/error handling.
3. Add FAT32 read-only directory/file browsing first; then controlled writes for installation.
4. Register display, keypad and storage devices with the device manager.
5. Add a small bitmap font and display text API, then a keypad-operated NOVA shell/launcher.
6. Add host tests for filesystem metadata and package validation before implementing application execution.
