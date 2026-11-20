# NOVA OS Alpha 0.9 — task stack safety budget

- Added a 480-byte high-water safety budget for each 512-byte task stack.
- Tasks whose measured high-water reaches the budget are quarantined with `NOVA_FAULT_STACK_LIMIT`.
- Shell task diagnostics display the configured stack budget and the fault command identifies stack-limit faults.
- Existing low-address guard words remain checked before and after each dispatch.

## Limits
The watermark is a diagnostic estimate, not an MPU guard or proof that every transient stack excursion was observed. A task may corrupt adjacent memory before the scheduler regains control. No automatic restart is attempted; the task remains FAULTED for inspection and explicit termination.

## Validation
No Alpha 0.9 workflow or physical-board test has been run yet.
