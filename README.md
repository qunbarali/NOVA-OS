# NOVA OS — Kernel Alpha 1.5

NOVA is an independent embedded OS project for Raspberry Pi Pico (RP2040 / Cortex-M0+) and Raspberry Pi Pico 2 (RP2350 / Cortex-M33). Alpha 1.5 integrates display, four-button UI, SDHC storage, FAT32, USB device mode and application-package verification foundations.

## Implemented source foundations

- Cooperative kernel, scheduler, IPC, device registry, fault capture and both Cortex-M architecture ports.
- ST7735 and ILI9341 RGB565 display drivers with selectable build profile.
- Batched 5x7 ASCII font rendering and a four-button graphical launcher.
- Four active-low buttons: Up, Down, Select and Back.
- Shared SPI0 bus arbitration for display and SD.
- SDHC/SDSC SPI block driver, 512-byte sectors, CSD capacity detection and 32 GiB capacity ceiling.
- FatFs R0.16 with long filename support (up to 255 characters), FAT12/16/32, serialized filesystem calls and synchronized atomic file replacement.
- TinyUSB CDC + MSC composite-device source integration. CDC is used for the shell; MSC exposes SD sectors.
- Exclusive ownership between the NOVA filesystem and USB host, plus eject/readiness handling.
- SHA-256 implementation and native known-vector test.
- On-device .nova package header, target, ABI, size and SHA-256 validation.

## Suggested prototype wiring

These GPIO assignments are a starting point. Match them to your actual modules and verify voltage levels before connecting.

| Peripheral | Signal | GPIO |
|---|---|---:|
| Shared SPI0 | SCK | GP18 |
| Shared SPI0 | MOSI | GP19 |
| Shared SPI0 | MISO | GP16 |
| Display | CS / D-C / RESET | GP13 / GP12 / GP11 |
| Display | Backlight | GP10 |
| MicroSD | CS | GP17 |
| Four-button module | Up / Down / Select / Back | GP2 / GP3 / GP4 / GP5 |

Default display: ST7735 128×160. Select ILI9341 240×320 using -DNOVA_DISPLAY_ILI9341=ON. Panel offsets, orientation, color order and clone-specific initialization still need physical testing.

## SD card and filesystem

- Target capacity: SDHC cards up to 32 GiB.
- Filesystem: FAT32 with long filenames. exFAT is not enabled.
- Format the card as FAT32 before use.
- The IDE copies packages into NOVA/APPS.
- The launcher can browse that directory and verify package header, target, ABI, payload length and SHA-256.
- A package marked verified is not executable yet: the firmware ELF segment loader, relocation checks, memory map and application lifecycle supervisor are still pending.

The IDE package maximum is 2 MiB total, including the 128-byte header. This is a storage/transport limit, not a guarantee that a 2 MiB executable fits in SRAM.

## USB behavior

The primary USB port currently has source support for:
- USB CDC ACM: shell and diagnostics.
- USB Mass Storage Class: SD-card block access from a connected host.
- Exclusive ownership: if NOVA has the filesystem mounted, MSC reports the medium not ready. NOVA releases storage only after filesystem sync/unmount; USB eject keeps the medium unavailable until disconnect.

The current VID/PID (0xCAFE:0x4011) is for development and must be replaced with an assigned identity before production. USB host mode, VBUS sourcing/overcurrent protection, Android/iPhone accessory protocols and firmware-update transport are not implemented. A USB-C connector also requires correct CC and power-path hardware; connector shape alone does not provide host/OTG capability.

## Battery

Battery telemetry is optional and disabled by default. To enable it, provide the actual ADC GPIO and measured resistor-divider multiplier using CMake options NOVA_BATTERY_ENABLE, NOVA_BATTERY_ADC_GPIO, NOVA_BATTERY_DIVIDER_NUMERATOR and NOVA_BATTERY_DIVIDER_DENOMINATOR.

The displayed percentage is a simple single-cell lithium voltage estimate, not a calibrated fuel-gauge reading. Charging and protection must be provided by a charger/power-path IC and battery protection circuit. Firmware does not charge the cell and does not yet implement a verified low-voltage shutdown policy.

## Windows NOVA IDE

Repository: https://github.com/qunbarali/NOVA-OS-IDE-

The WPF IDE has project build controls, RP2040/RP2350 target selection, .nova package creation and inspection, and a FAT32 SD-card copy operation into NOVA/APPS. The package writer accepts ARM ELF32 (requiring a nova_app_main Thumb entry symbol) or raw BIN with an explicit load address. UF2 is reserved for firmware flashing/recovery.

The firmware ABI v1 service table is present, and the IDE includes a starter ABI header. The dynamic app loader, target-specific linker memory map, relocation validation, execution isolation and full lifecycle handling are not complete. Do not expect installed packages to run yet.

## Build and validation

The manual-only GitHub Actions workflow now has four cross-build variants:
- RP2040 + ST7735
- RP2040 + ILI9341
- RP2350 Cortex-M33 + ST7735
- RP2350 Cortex-M33 + ILI9341

It also runs Python source-contract tests and native scheduler, IPC, device, storage-owner and SHA-256 tests. The workflow has not been triggered for the latest Alpha 1.5 changes. No physical panel, SD-card, USB-host or battery validation is claimed.

To run it: GitHub → Actions → NOVA OS firmware build → Run workflow. No push or pull-request trigger is configured.

## Hardware validation still required

1. Verify actual module voltage and pin labels before connecting SPI, display and buttons.
2. Test ST7735 and ILI9341 initialization, orientation, offsets and color order.
3. Test FAT32 cards at multiple capacities, long filenames, power-loss atomic writes and repeated mount/eject.
4. Test Windows CDC shell and MSC copy/eject, including simultaneous-use refusal.
5. Confirm USB-C CC, VBUS sensing and power-path behavior on the actual carrier board.
6. Provide charger IC, battery chemistry, divider values and gauge details before enabling battery telemetry.
7. Complete ELF loader and lifecycle work after cross-build map files establish safe app RAM regions.

See docs/ALPHA_1_5_STORAGE_USB_UI.md, docs/APP_RUNTIME_DESIGN.md, docs/USB_AND_BATTERY_ARCHITECTURE.md, docs/ALPHA_1_4_DEVELOPMENT.md, docs/ARCHITECTURE.md and docs/PHASE1_TEST_PLAN.md.
