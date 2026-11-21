# NOVA Alpha 1.5 — filesystem, USB and launcher integration

## Implemented source foundations
- FatFs R0.16 vendored with LFN enabled (255 UTF-16 code-unit maximum, ANSI code-page API) and fixed single-owner operation model.
- SD disk I/O adapter uses the existing SPI SDHC/SDSC block driver.
- FAT volume is mounted only after acquiring the kernel storage owner; unmount synchronizes the disk before releasing ownership.
- Atomic file replacement writes a temporary file, calls f_sync, closes it, moves an existing target to a backup, renames the temporary file into place and syncs the disk. A later replacement attempt restores a backup if a previous replacement was interrupted before the final rename.
- TinyUSB composite descriptors expose CDC and MSC interfaces. CDC is registered as the Pico stdio driver. MSC reads/writes 512-byte SD sectors.
- The USB MSC owner and kernel filesystem owner are mutually exclusive. If the filesystem is mounted, the host receives NOT READY; once the OS unmounts, a subsequent host readiness request may claim the medium. Eject marks the medium unavailable until USB disconnect.
- A graphical launcher, batched 5x7 ASCII font renderer, four-button navigation, SD root browser and installed-package catalog at `0:/NOVA/APPS` are wired into the main loop. Selecting an app validates its package and reports that no runtime loader is available; it does not execute the package.
- Battery ADC telemetry is compile-time disabled by default. Optional configuration requires a measured divider ratio and ADC GPIO; ADC readings are averaged over 16 samples.

## FAT32 / LFN configuration
The current FatFs configuration uses FAT12/16/32 support and LFN. exFAT is not enabled. A 32 GB SDHC card should be formatted FAT32; exFAT-formatted cards are not supported by this configuration. The launcher displays a truncated root listing for screen fit, but filesystem path APIs preserve the full FatFs path capability.

## USB details
The composite device uses a development VID/PID (0xCAFE:0x4011) and must not be treated as a production-assigned USB identity. Replace it with a properly licensed VID/PID before distributing hardware. USB is device mode. Host mode for peripherals such as Android/iPhone accessories is a separate role and requires VBUS sourcing, power switching, protection and role negotiation; it is not implemented here.

## Battery hardware
The ADC code is disabled unless configured with NOVA_BATTERY_ENABLE and a measured divider multiplier. The current percentage is a simple voltage estimate for a single-cell lithium battery, not a fuel-gauge measurement. It must not control charging or be used as a safety cutoff. Charging requires an external charger/power-path IC and appropriate protection. Do not connect a battery directly to an RP-series GPIO.

## Still not complete
- Dynamic NOVA application ELF loader, relocation policy, ABI syscall boundary, memory isolation and lifecycle supervisor.
- Application install/catalog workflow beyond removable-drive file transfer.
- USB host mode / Android and iPhone accessory protocol support.
- Battery charger IC integration, fuel gauge calibration and hardware low-voltage cutoff.
- Automated cross-compilation results for this commit series and physical hardware validation.

## Validation
The firmware workflow remains manual-only. Source changes and native ownership tests are committed, but neither RP2040 nor RP2350 firmware build has been run for this milestone. The display panel offsets, SD module voltage/CS wiring, USB enumeration and power behavior must be checked on the actual hardware.
