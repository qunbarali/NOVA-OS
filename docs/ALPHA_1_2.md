# NOVA OS Alpha 1.2 — device manager and HAL registration

- Added a fixed-capacity registry for up to eight named devices.
- Each registered device declares a class, read/write/control capabilities, operations table and driver context.
- Registration validates names, classes, callbacks and duplicate names; device inventory is available through a bounded snapshot API.
- Device I/O copies the descriptor while holding the shared spinlock, then calls the driver outside the lock.
- Registered the board's onboard LED as `led0` with one-byte read/write operations.
- Added shell commands `devices`, `device led0 on` and `device led0 off`.
- Added a host test for registration, duplicate rejection, capability-based reads/writes and inventory snapshots.
- Extended the manual workflow with a native device-manager test stage.

## Scope and limitations
This is a static early device registry, not a complete Plug-and-Play manager or security capability system. It has no unregister/hot-plug lifecycle, driver reference counting, DMA mapping, interrupt routing, block-device implementation, filesystem, USB host stack or storage driver. Only the onboard LED is registered by the current board port.

## Validation
Alpha 1.2 has not yet been compiled by the manual dual-board workflow or tested on RP2040/RP2350 hardware. The workflow remains manual-only; run it after reviewing the commits.
