# USB and battery power architecture

## Product requirement
The primary board USB connector (USB-C or USB-B/micro-B depending on the carrier PCB) is the normal NOVA data, diagnostics, recovery and charging connection. Connector shape does not determine host/device role; the PHY/controller configuration, CC/ID wiring, VBUS sensing and power switches do.

## Normal device mode
The default USB role is USB device (peripheral), implemented with the Pico SDK TinyUSB device stack:
- USB Mass Storage Class (MSC): export the SD card as a removable drive to a connected Windows/Linux/Android host where that host supports USB storage.
- USB CDC ACM: serial console, diagnostics and NOVA IDE control protocol.
- A future vendor-specific interface may provide IDE operations that cannot safely be expressed through CDC.
- Firmware recovery is a separate, guarded update mode. UF2 is a firmware image/transport, not an ordinary NOVA application package.

### Storage ownership and data integrity
When the SD card is exported as MSC, the external host owns the filesystem. NOVA must not concurrently mount, modify, launch or write files on that same FAT volume. The OS must:
1. stop application/storage writes;
2. flush pending writes and unmount its filesystem;
3. expose the raw block device to MSC;
4. wait for host eject/stop or disconnect;
5. reinitialize and remount the volume before local access resumes.

If a safe ownership handoff cannot be confirmed, expose storage read-only or refuse MSC attachment. Never allow simultaneous host and NOVA writes to FAT32.

## USB host mode
The RP2040/RP2350 USB controller can operate in host or device mode, but the native controller cannot run both roles concurrently. A host port requires appropriate connector wiring, 5 V VBUS source with current limiting/overcurrent protection, role detection and a safe power switch. A second simultaneous host port can be explored using PIO USB, subject to PIO, timing and CPU budget. The primary connector must not source 5 V to another device while connected as a USB device to a PC.

The firmware currently implements the USB device-mode CDC + MSC composite source foundation. Host mode is still not implemented and must be enabled only after the carrier board includes a correctly wired host/OTG power path. USB host support for keyboards, mice, flash drives and phones is a separate phase; it is not implied by the presence of a USB-C receptacle.

## Phone interoperability
- Android: MSC drive access can work when the phone supports USB host/OTG and the file system is supported. CDC/vendor protocols need an Android app and appropriate USB permission.
- iPhone/iPad: external-storage access depends on the model, connector/adapter, iOS version and supported file system/accessory behavior. A dedicated iOS app/protocol is a separate project; generic USB serial access is not guaranteed.
- Windows/Linux: MSC and CDC are standard interfaces; the IDE can use CDC for control and MSC for user file transfer.

## Battery and charging
Charging is primarily a hardware power-management responsibility, not a GPIO firmware feature. Carrier board requirements:
- single-cell Li-ion/LiPo charger with proper charge termination and battery protection;
- power-path/load-sharing management so the board can run while charging;
- USB input current limiting and thermal design;
- battery connector polarity protection;
- battery voltage/current/temperature monitoring, ideally via a fuel-gauge IC;
- a resistor divider/ADC or gauge interface that keeps all MCU pins within their electrical limits;
- USB-C sink CC pull-down resistors (Rd) on CC1 and CC2 when using a USB-C receptacle as a USB 2.0 device/power sink; do not connect CC pins directly to GPIO as a substitute for compliant Type-C circuitry.

Do not connect a Li-ion cell directly to the Pico VSYS/3V3 rail or a GPIO. Do not attempt battery charging through firmware PWM. Firmware should report state-of-charge/voltage/temperature and implement low-battery warnings/shutdown only after the selected charger/gauge and board pinout are known.

## Firmware integration sequence
1. SD SPI block driver and card capacity detection (implemented; no physical card-detect signal).
2. FatFs FAT12/16/32 with LFN and serialized kernel ownership (source implemented; not yet hardware tested).
3. TinyUSB CDC + MSC composite device using the SD block driver (source implemented; not yet cross-built).
4. Exclusive MSC ownership and eject handling (source implemented; host behavior still needs validation).
5. Firmware update/recovery protocol with image signature/hash and board/size checks (pending).
6. Battery telemetry matched to the actual gauge/ADC hardware (optional ADC estimate implemented but disabled; hardware-specific setup pending).
7. USB host mode after carrier-board power/role hardware is verified (pending).

## Current implementation status
This document specifies the intended behavior. USB MSC/CDC composite, SD block layer, battery telemetry and USB host role are not yet implemented in NOVA firmware. Alpha 1.5 now includes a TinyUSB CDC + MSC composite implementation, SD block callbacks, an exclusive kernel/USB storage ownership gate, FatFs LFN support, synchronized atomic file replacement and optional ADC battery telemetry (disabled by default). The USB development VID/PID is 0xCAFE:0x4011 and must be replaced before production. USB host role, charger/power-path integration, fuel-gauge support, firmware update protocol, and physical host/device interoperability validation remain incomplete. The MSC/CDC implementation has not yet passed a fresh RP2040/RP2350 cross-build or hardware test.
