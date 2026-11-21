# Alpha 1.4 development — SD block storage and USB platform groundwork

## Implemented in this development milestone
- Shared SPI bus mutex for serialized peripheral transactions.
- ST7735/ILI9341 display operations now hold the shared bus for complete display transactions.
- SPI-mode SD card initialization (CMD0, CMD8, ACMD41, CMD58, CMD16 where required).
- SD CSD capacity decoding and 512-byte sector count.
- Single/multi-sector block read and write API using CMD17/CMD24.
- SDHC block addressing and SDSC byte addressing.
- Bounds checks against detected card sector count.
- CMake includes the SPI bus and SD block layer.
- Source-contract checks cover the SPI/SD interfaces.

## Hardware target
The requested storage target is SDHC/FAT32 cards up to 32 GB. The current code is a low-level SPI block driver, not a FAT32 filesystem. Card compatibility, SPI speed, power integrity and write behavior must be validated with the actual module and card.

Suggested shared SPI0 wiring (confirm module pin labels):
- GP18 SCK, GP19 MOSI, GP16 MISO
- Display CS GP13, SD CS GP17
- Display D/C GP12, display reset GP11, optional backlight GP10
- Keypad active-low inputs GP2–GP5

## Still required before SD can be used by applications
- FAT32 mount, directory enumeration, long filename support and file read/write.
- Filesystem flush/eject and single-owner lock.
- Device manager registration and startup configuration.
- Host tests for FAT metadata and card error paths; physical card tests.

## USB and battery
See [USB and battery architecture](USB_AND_BATTERY_ARCHITECTURE.md). The primary USB device mode is planned as MSC + CDC, with MSC exposing the SD card only after safe filesystem ownership handoff. Host mode is separate and requires host VBUS/power hardware. Battery charging requires a charger/power-path IC; firmware telemetry depends on the selected gauge or divider wiring.

## Validation status
No firmware build or hardware test is claimed by this commit series. The manual GitHub Actions workflow has not been triggered. SD SPI code and display bus serialization need cross-compilation and real-card/panel testing before release.
