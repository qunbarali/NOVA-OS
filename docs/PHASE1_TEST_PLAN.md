# NOVA Alpha 0.2 — Build and Hardware Test Plan

## A. Build matrix

| Target | Board selection | Expected output |
|---|---|---|
| RP2040 | PICO_BOARD=pico | build-pico/nova_os.uf2 |
| RP2350 ARM | PICO_BOARD=pico2 | build-pico2/nova_os.uf2 |

Build each target in a clean, separate directory. Record compiler, SDK and CMake versions with every release candidate.

## B. Firmware bring-up checks (each board)

1. UF2 appears on the BOOTSEL RPI-RP2 volume and copies successfully.
2. Board reboots after UF2 installation.
3. USB CDC serial port enumerates.
4. version returns Alpha 0.2.0.
5. system reports board family, chip, board ID, uptime, timer tick count and timer health.
6. memory reports physical SRAM capacity and fixed-pool free blocks.
7. cpu reports core 1 ready.
8. tasks reports task count and dispatch count.
9. task demo creates a LED pulse task; tasks shows it sleeping/running over time.
10. task stop removes the demo task and turns the LED off.
11. led on and led off operate the onboard LED.
12. Long command input is rejected without buffer overflow; backspace works.
13. reboot restarts the firmware.
14. Leave running for at least 30 minutes and check for lockups or unexpected resets.

## C. Kernel service test cases

The committed Python tests are source-contract checks. Add/run runtime tests for the following before declaring the scheduler production-ready:

- Create tasks up to the 8-slot limit; ninth create must return invalid.
- Verify round-robin polling visits every READY task exactly once per scheduler pass.
- Verify sleeping task does not run before its deadline and wakes at/after deadline, including uint32 tick wraparound.
- Verify suspend/resume/terminate state transitions and invalid task IDs.
- Verify a task returning EXIT releases its slot.
- Allocate all eight pool blocks; ninth allocation must fail; free and reallocate blocks.
- Reject freeing null, foreign and already-freed pool pointers.
- Queue accepts eight messages, rejects the ninth, preserves payload length and FIFO order.
- Queue receive with too-small destination buffer must fail without consuming the message.
- Mutex rejects calls outside a scheduled task, rejects double lock, and rejects unlock by a non-owner task.
- Event set/get/clear preserves unrelated flag bits.

## D. Test evidence to record

- Board model and revision
- Firmware build commit/hash
- Pico SDK version
- Toolchain version
- Build logs and warnings for both targets
- USB COM port enumeration
- Serial output for each command
- Timer tick progression
- Task demo behavior
- 30-minute stability result

## E. Exit criteria

Alpha 0.2 source implementation is committed. Do not call the milestone build-verified until both firmware targets compile. Do not call it hardware-validated until all board checks above pass on both physical boards and runtime kernel-service tests have been recorded.

## F. Safety

Do not write RP2350 OTP during development. Do not modify flash partitioning, boot metadata or bootloader regions in this milestone. Keep the ROM BOOTSEL recovery path available.
