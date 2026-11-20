# NOVA OS Alpha 0.8 — Cortex-M HardFault capture

- Added architecture-specific HardFault entry stubs for Cortex-M0+ and Cortex-M33.
- Captures the basic exception frame (PC, LR, xPSR, R0/R1, frame address and EXC_RETURN).
- Handles the optional extended floating-point frame offset on the M33 path.
- Stores a compact record in watchdog scratch registers and requests a watchdog reboot; inspect after restart with `crash`.
- `crash clear` clears the retained record.

## Limits
This is a minimal HardFault diagnostic path, not a complete crash dump. It does not decode CFSR/HFSR, capture all registers, recover from arbitrary memory corruption, or guarantee capture when the exception stack itself is invalid. Watchdog scratch retention is reset-class dependent and must be confirmed on hardware. No fault injection or board validation has been performed.

## Validation
The manual dual-board workflow remains the next validation step. Source inspection alone does not establish hardware correctness.
