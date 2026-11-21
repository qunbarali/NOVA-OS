# NOVA application ABI and runtime design

## Package and storage contract
- The Windows IDE produces a .nova package with a 128-byte header and SHA-256 payload digest.
- The package is limited to 2 MiB total. The payload is stored on the SD card; this is not a promise that a 2 MiB executable fits in SRAM.
- UF2 remains a firmware/recovery format and is not an application image.
- The Alpha IDE accepts little-endian ARM ELF32 and raw BIN. BIN requires an explicit load address.

## ABI v1 proposal in source
The C header include/nova/app_abi.h defines the ABI version, entry symbol, service-table layout and lifecycle states. The entry contract is nova_app_main(const nova_app_api_v1_t *api). An app must verify api->abi_version and api->struct_size before using services.

The firmware now exports the v1 service table for uptime, display text, SD mount/unmount, file reads, atomic file writes, cooperative yield and stop requests. The runtime-side application loader, task lifecycle integration and actual invocation path are not yet complete.

## Loader safety requirements
1. Validate package signature, format version, target ID, ABI version, exact payload length and SHA-256 before loading.
2. Parse ELF32 headers with bounds-checked arithmetic. Accept only ARM little-endian executable images with supported PT_LOAD segments.
3. Reject dynamic linking, unsupported relocation types, overlapping segments, out-of-range addresses, malformed alignment and entry points outside executable segments.
4. Load only into a dedicated app memory region defined by the final linker map. Zero BSS and validate stack bounds.
5. Run one application at a time initially. Provide stop, return, fault and watchdog recovery paths.
6. Keep kernel and USB storage services unavailable or serialized while an app owns a file handle.
7. Do not allow a package to choose arbitrary privileged memory addresses.

## Why native app execution is not enabled yet
The firmware has not yet produced linker maps for both RP2040 and RP2350 after the new FatFs/TinyUSB additions. Until actual static RAM use and stack budgets are measured, reserving an application RAM window would be guesswork and could overwrite kernel state. RP2040 has 264 KiB SRAM and RP2350 has 520 KiB SRAM, but the usable application region must be calculated from the final linker map. The 2 MiB package limit is a storage/transport limit; application loadable segments must fit the reserved execution region or use a future flash-backed design.

The ABI header and IDE package UI are initial foundations only. Do not install or execute untrusted .nova packages until the runtime loader, target validation, memory bounds and lifecycle supervisor are implemented and tested.
