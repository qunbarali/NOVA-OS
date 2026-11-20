#include "nova/critical.h"
#include "hardware/sync.h"

nova_irq_state_t nova_critical_enter(void) {
    return save_and_disable_interrupts();
}

void nova_critical_exit(nova_irq_state_t state) {
    restore_interrupts(state);
}
