# NOVA OS Alpha 0.7 — bounded task fault journal

## Delivered
- Added a bounded eight-record task fault journal.
- Records sequence number, scheduler tick, task ID, task name and fault reason.
- Records stack-guard corruption, invalid callback results and callbacks that return while still marked RUNNING.
- Keeps the total fault counter independent of retained journal capacity.
- Added a read-only snapshot API and a shell `faults` command.
- Added native coverage for invalid-result fault capture and record identity.
- Bumped firmware version to 0.7.0.

## Scope and safety
This milestone records faults detected by the cooperative task scheduler. It does **not** install Cortex-M HardFault/MemManage/BusFault/UsageFault handlers, decode exception frames, or recover from arbitrary CPU exceptions. A fault during exception entry or on a corrupted active stack can still prevent normal logging. The journal is volatile RAM and is cleared on reboot.

## Validation
- Native test and source-contract coverage has been added.
- The manual dual-board workflow has not been run for Alpha 0.7.
- No physical RP2040 or RP2350 validation is claimed.
