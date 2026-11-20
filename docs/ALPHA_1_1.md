# NOVA OS Alpha 1.1 — blocking IPC and task wait queues

- Added a BLOCKED task state and timeout-aware task block preparation/commit APIs.
- Added atomic wake-pending bits so another core can request a wake without directly modifying scheduler-owned task state.
- Added blocking queue send and receive with optional timeouts and bounded waiter bitmaps.
- Added event-flag waits with optional clear-on-exit and timeout support.
- Queue/event producers wake eligible waiters; the core-0 scheduler applies wake requests and resumes task contexts.
- Added host scheduler coverage for blocking, timeout and explicit wake; added native IPC tests to the manual firmware workflow.

## Concurrency contract
Blocking APIs must be called from a running core-0 task. The scheduler and task table remain core-0-owned. Core 1 may use the spinlock-protected nonblocking queue/event APIs and request task wakeups, but must not call task creation/management or blocking APIs. No IPC API is ISR-safe.

## Validation
The manual dual-board workflow has not been run for Alpha 1.1. Native tests are added to the workflow but have not yet been executed in this task.
