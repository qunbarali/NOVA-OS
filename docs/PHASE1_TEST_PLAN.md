# NOVA Phase 1 — Build and Hardware Test Plan

## A. Build matrix

| Target | Board selection | Expected output |
|---|---|---|
| RP2040 | `PICO_BOARD=pico` | `build-pico/nova_os.uf2` |
| RP2350 ARM | `PICO_BOARD=pico2` | `build-pico2/nova_os.uf2` |

Build each target in a clean, separate directory. Record compiler, SDK and CMake versions with every release candidate.

## B. Bring-up checks (each board)

1. UF2 appears on the BOOTSEL `RPI-RP2` volume and copies successfully.
2. Board reboots after UF2 installation.
3. USB CDC serial port enumerates.
4. `version` returns Alpha 0.1.1.
5. `system` reports the correct board family, chip and board ID.
6. `memory` reports 264 KiB on RP2040 or 520 KiB on RP2350; this is total SRAM capacity, not free RAM.
7. `cpu` reports core 1 ready.
8. `tasks` heartbeat counter advances between commands.
9. `led on` and `led off` operate the onboard LED.
10. Long command input is rejected without buffer overflow; backspace works.
11. `reboot` restarts the firmware.
12. Leave running for at least 30 minutes and check for lockups or unexpected resets.

## C. Test evidence to record

- Board model and revision
- Firmware build commit/hash
- Pico SDK version
- Toolchain version
- Build log and warnings
- USB COM port enumeration
- Serial output for each command
- 30-minute stability result

## D. Exit criteria for Alpha 0.1

Both board targets compile cleanly, install through UF2, enumerate USB CDC, pass all command checks and complete the stability test. Do not call Alpha 0.1 hardware-validated until this evidence exists for both physical boards.

## E. Safety

Do not write RP2350 OTP during development. Do not modify flash partitioning, boot metadata or bootloader regions in this milestone. Keep the ROM BOOTSEL recovery path available.
