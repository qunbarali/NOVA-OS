# NOVA OS Alpha 0.5 — Resumable cooperative context switching

## Delivered in source
- Each task retains its C call chain on its own 512-byte stack while yielding or sleeping.
- `nova_task_yield()` resumes at the instruction after the call on a later scheduler dispatch.
- `nova_task_sleep_current(ms)` resumes after its wrap-safe deadline at the instruction after the sleep call.
- Returning `NOVA_TASK_AGAIN` rebuilds the task stack and restarts the callback; `NOVA_TASK_EXIT` terminates it.
- Separate assembly context-switch ports save and restore the callee-saved register sets for Cortex-M0+ and Cortex-M33. Task code runs on PSP; the scheduler retains its saved MSP context and a dedicated 1 KiB MSP stack serves exception handlers while tasks run.
- Initial task stack frames are constructed for each architecture's restore order.
- Scheduler state and each task's saved stack pointer are maintained separately.

## Execution contract
Tasks are cooperative. A callback that never yields, sleeps, or returns can monopolize core 0. Tasks share the same address space and privilege level; this is not preemptive scheduling or process isolation.

The native host test mode (`NOVA_HOST_TEST`) executes callbacks directly to test scheduler policy. It does not emulate assembly context switching or prove suspended C stack restoration.

## Validation limits
- The 512-byte stack is a bounded development default, not suitable for arbitrary deep call chains.
- Four guard words detect only writes that reach the guard; this is not memory protection.
- Both assembly ports require the manual cross-build and physical RP2040/RP2350 testing.
- Exception entry, fault stack capture, interrupt nesting policy and preemptive switching remain future work.
- Kernel task and IPC data remain core-0-owned; core 1 is still only a diagnostic heartbeat.

## Hardware checklist
1. Build both `pico` and `pico2` targets.
2. Flash each UF2 through the board's ROM UF2 path.
3. Connect USB CDC; check `version`, `system` and `tasks`.
4. Run `task demo`; verify LED toggles at roughly 500 ms intervals.
5. Test local variables around repeated yield/sleep calls and verify each task resumes rather than restarting.
6. Inspect stack high-water readings and fault count.

No physical hardware test is claimed by this source milestone.
