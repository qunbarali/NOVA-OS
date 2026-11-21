# NOVA-OS display and application platform plan

## Display support
NOVA-OS provides a common RGB565 display API with selectable controller:
- ST7735: configure actual visible dimensions and X/Y offsets. Common panels are 128x160; module variants differ.
- ILI9341: configure actual visible dimensions and offsets. Common panels are 240x320.
- Both use SPI and independent chip-select, data/command and reset signals. Backlight is optional.
- Display and SD storage may share one SPI peripheral only when each has a separate chip-select and the drivers release the bus between transactions. The initial display driver owns SPI transactions directly; shared-bus arbitration is required before enabling SD and display concurrently.
- Orientation, RGB/BGR order, ST7735 initialization variant and offsets must be board-configurable. Initial command sequences are starting profiles, not validation for every clone module.

## Windows IDE and NOVA application format
A separate Windows desktop IDE is required. It should provide project creation, source editing, target selection (RP2040 / RP2350 Cortex-M33), build output, diagnostics, package inspection, and USB/SD deployment.

The canonical installable format should be a NOVA package (.nova) containing:
- magic and format version;
- target chip/architecture and minimum NOVA ABI version;
- application name, version, entry point and permissions/capabilities;
- image type, load-address policy, image length and checksum/hash;
- optional resources and metadata.

The package format, not the filename extension, defines executable semantics. The IDE may import/export ELF and raw BIN when the image is compatible with the NOVA ABI and loader. The OS must reject unknown, oversized, malformed, wrong-target, overlapping or checksum-invalid images before installation.

## UF2 distinction
UF2 is a bootloader transport/flash image format. A UF2 containing a complete NOVA firmware image is installed through a controlled firmware update/recovery flow, not launched as an SD-card application. The IDE may generate UF2 for flashing NOVA firmware or supported firmware bundles. It must not imply that arbitrary UF2 can run as a process.

## 2 MiB application limit
The first release should enforce a maximum package/image file size of 2 MiB on SD. This is a storage limit, not a RAM guarantee. The loader must separately validate RAM/flash placement, stack, heap, ABI and board memory constraints. RP2040 has 264 KiB SRAM; RP2350 has 520 KiB SRAM.

## Execution model requirement
The current NOVA kernel is cooperative and shares one address space. It does not yet have process isolation, an application ABI, relocation/loading support, or safe user-mode boundaries. The initial application milestone must first define a restricted, statically linked application ABI and safe launch/return contract. Do not execute arbitrary uploaded ELF/BIN files until these are implemented and tested.

## Hardware pin proposal
Shared SPI prototype: GP18 SCK, GP19 MOSI, GP16 MISO; display CS GP13, SD CS GP17; display D/C GP12, reset GP11, optional backlight GP10. Four keypad inputs may use GP2-GP5 with pull-ups and active-low switches. Confirm voltage levels and module pin labels before connecting hardware.
