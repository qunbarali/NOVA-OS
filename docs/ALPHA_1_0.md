# NOVA OS Alpha 1.0 — cross-core shared-service locking

- Added a shared RP hardware spinlock service initialized before core 1 starts.
- Applied the lock to bounded message queues, task-owner mutexes, event flags and the fixed memory pool.
- Preserved local interrupt masking as a separate short critical-section primitive.
- Added source-contract checks for lock initialization and protected service use.

## Concurrency boundary
The shared data structures above are protected for concurrent core access. This does not make the task scheduler or task-control table SMP-safe. Core 1 remains a service/worker core and must not call task-management APIs. These spinlock APIs are not ISR-reentrant; do not call them from interrupt handlers.

## Validation
No Alpha 1.0 dual-board workflow or hardware stress test has been run yet.
