# NOVA Windows IDE — product and engineering specification

## Purpose
NOVA IDE is the official Windows desktop development environment for applications that run under NOVA-OS on Raspberry Pi Pico (RP2040) and Pico 2 (RP2350 Cortex-M33). It is a separate developer tool from the embedded operating system.

## Proposed Windows implementation
- Windows 10/11 x64 desktop application.
- C# and .NET 8 with WPF for the native desktop shell.
- Project/build engine invokes pinned ARM GNU Embedded Toolchain, CMake and Ninja without requiring users to operate a terminal.
- Project files remain human-readable and portable; the IDE is not allowed to silently rewrite source files.
- Offline-first operation. Toolchain packages are installed or selected locally; no cloud account is required to build.

## IDE workspaces
1. Project Explorer: source, headers, assets, libraries and manifest.
2. Code Editor: syntax highlighting, diagnostics, search and project-wide navigation.
3. Board & Build: RP2040 / RP2350 Cortex-M33, debug/release, memory map, compiler output.
4. NOVA API browser: display, keypad, storage, filesystem, GPIO, IPC and application lifecycle APIs.
5. Resource Designer: image/font conversion and resource-size inspection for constrained displays.
6. Package Inspector: inspect target, ABI, entry point, sections, RAM/flash requirements, hash and 2 MiB package limit.
7. Deploy: copy .nova packages to SD storage, or enter a clearly separate firmware-recovery workflow for UF2.
8. Serial Monitor: USB CDC console, build/deploy status and runtime diagnostics.

## Output and extension rules
- Canonical runnable application package: .nova. The package header and manifest determine format; the extension is only a file association.
- ELF: build/debug artifact and supported import/export format only when compatible with NOVA's declared ABI and loader constraints.
- BIN: raw image import/export only with explicit target, address/relocation policy, ABI and checksum metadata.
- UF2: firmware flashing/recovery artifact. Never label arbitrary UF2 as an SD-launched process.
- The IDE may accept source projects regardless of their original folder or source-file extensions, but every produced binary must be validated against the selected target and NOVA ABI.
- Maximum installable package file size: 2 MiB in the first release. This does not imply that the entire package is loaded into SRAM.

## Build pipeline
1. Validate project manifest and selected target.
2. Compile and link against the versioned NOVA application SDK.
3. Inspect ELF sections, symbols, relocations, entry point and memory requirements.
4. Reject unsupported relocations, privileged/system-region writes, invalid address ranges and incompatible ABI.
5. Convert supported image into a .nova package with manifest, payload length and cryptographic hash/checksum.
6. Re-open and independently verify the package before presenting it as installable.
7. Optionally export ELF/BIN for engineering and UF2 only for firmware deployment.

## Runtime prerequisites
NOVA-OS must implement a stable application ABI, loader, lifecycle manager, memory accounting and safe return/fault handling before it executes third-party applications. The current kernel has cooperative tasks sharing one address space and has no process isolation. Early applications must therefore be statically linked, bounded, trusted NOVA SDK programs; arbitrary ELF/BIN execution is out of scope until isolation and validation exist.

## Release sequence
- IDE Alpha 0: project format, toolchain detection, build console and target selection.
- IDE Alpha 1: NOVA SDK templates, package manifest and package validator.
- IDE Alpha 2: deploy .nova files to SD and list/install from NOVA-OS.
- IDE Alpha 3: source debugger integration and serial monitor.
- IDE Alpha 4: firmware UF2 recovery/export kept separate from application deployment.
