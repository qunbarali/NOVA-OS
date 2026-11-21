# NOVA-OS display and application platform

## Display support implemented
NOVA has a common RGB565 SPI display API with selectable controller profiles:
- ST7735: default profile 128x160; module-specific offsets and init sequences may differ.
- ILI9341: selectable profile 240x320.
- Both use independent chip-select, D/C and reset signals, with optional backlight.
- Display and SD share SPI0 through the bus mutex.
- A batched 5x7 ASCII font renderer and a graphical launcher are integrated.
- The four active-low buttons navigate the launcher and SD/application catalog.

Orientation, RGB/BGR order, panel offsets, clone-specific init, actual voltage levels and SPI timing still require physical panel testing.

## Storage and USB
The SD SPI block driver supports SDHC/SDSC, 512-byte sectors and CSD capacity detection up to 32 GiB. FatFs R0.16 provides FAT12/16/32 and long filenames. exFAT is disabled. Filesystem operations are serialized and the filesystem owner is mutually exclusive with USB MSC.

TinyUSB device mode exposes:
- CDC ACM for the NOVA shell and diagnostics.
- MSC for raw SD block access to a connected Windows/Linux/Android USB host that supports mass storage.

NOVA must unmount and release the filesystem before the host can own the medium. USB eject keeps the SD unavailable until disconnect. USB host mode, VBUS sourcing, role switching and iPhone accessory protocol support remain future work.

## Windows IDE and NOVA application format
The Windows WPF IDE lives in a separate repository: https://github.com/qunbarali/NOVA-OS-IDE-

The IDE can build a CMake project, package compatible ARM ELF32 or raw BIN into a .nova container, inspect package hashes, and copy packages to a FAT32 removable SD card under NOVA/APPS. UF2 is reserved for firmware flashing/recovery and is not an application.

The package format includes a 128-byte header with signature, version, target, ABI version, image kind, load address, name, version, payload length and SHA-256. The total package size is limited to 2 MiB. This is a storage/transport limit, not an executable RAM limit.

## ABI and runtime status
The firmware defines ABI v1 service-table declarations and implements service functions for uptime, display text, SD mount/unmount, file read, atomic file write, cooperative yield and stop request. The IDE validates the ELF32 executable type and nova_app_main Thumb entry symbol. Firmware validates package target, ABI, size and payload SHA-256 before reporting it as verified.

The dynamic ELF loader is not yet implemented. It must validate program headers and PT_LOAD ranges, reject unsupported relocations, define target-specific app RAM regions from real linker maps, initialize BSS and stack, and run a cooperative lifecycle supervisor. RP2040 has 264 KiB SRAM and RP2350 has 520 KiB SRAM, but the usable app memory budget must be calculated from the final Alpha firmware map. A 2 MiB package cannot be assumed to fit in RAM.

Do not execute arbitrary packages until the loader, memory boundaries, lifecycle handling and fault recovery have been implemented and validated.

## Battery hardware
Battery ADC telemetry is optional and disabled by default. The CMake configuration requires a measured voltage-divider ratio and an ADC GPIO. The current estimate is based on single-cell lithium voltage and is not a fuel gauge. A charger/power-path IC, battery protection and hardware low-voltage cutoff are external hardware responsibilities. Do not connect a battery directly to an RP-series GPIO.

## Prototype pin proposal
Shared SPI0: GP18 SCK, GP19 MOSI, GP16 MISO; display CS GP13, SD CS GP17; display D/C GP12, reset GP11, optional backlight GP10. Four keypad inputs GP2-GP5 with pull-ups and active-low switches. Confirm the actual module labels and electrical requirements before connecting.
