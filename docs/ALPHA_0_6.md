# NOVA OS Alpha 0.6 — Stack diagnostics hardening

## Delivered
- Corrected task stack high-water accounting for downward-growing Cortex-M stacks.
- The watermark scanner starts at the low address of the task stack and finds the first word no longer matching the fill pattern. Used space is measured from that point to the stack top.
- Initial task context frames are included in the baseline usage, so a newly created task reports the space occupied by its saved register/return frame rather than zero.
- A fresh callback invocation (NOVA_TASK_AGAIN) rebuilds its stack and resets the watermark to the initial-frame baseline. Yielding and sleeping tasks retain their existing stack and watermark.
- Shell task diagnostics explicitly label the reported value as a high-water measurement.
- Existing four-word guard checks and FAULTED task state remain in place.

## Why this matters
The previous scanner walked from the high end of the stack downward. Since the initial context frame is always non-fill, it stopped immediately and failed to observe deeper stack use below that frame. The corrected direction detects the deepest touched address for a downward-growing stack.

## Validation
- Native scheduler test checks the initial context-frame baseline (nine 32-bit words / 36 bytes).
- Source-contract test checks the low-address-first scan and byte calculation.
- The Alpha 0.5 manual GitHub workflow passed both firmware builds, native tests and source-contract tests. Alpha 0.6 has not yet been run through that workflow.
- No physical RP2040 or RP2350 validation is claimed.

## Still not provided
- Stack guard words are diagnostics, not memory protection.
- Stack high-water readings are estimates based on a fill pattern and can be affected by compiler/runtime stack writes.
- No MPU-based per-task isolation, exception-frame capture, preemptive scheduler, or cross-core kernel locking is included.
