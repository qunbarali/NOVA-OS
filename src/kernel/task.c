/* Phase 1 task subsystem placeholder.
 * Alpha 0.1 uses a foreground event loop and a dedicated core-1 heartbeat.
 * A bounded cooperative task queue is planned for Alpha 0.2; preemption is
 * deliberately deferred until stack ownership and interrupt rules are tested.
 */
#include <stdint.h>
uint32_t nova_task_subsystem_abi(void) { return 1u; }
