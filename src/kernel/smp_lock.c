#include "nova/smp_lock.h"
#include "hardware/sync.h"

static spin_lock_t *kernel_lock;
static bool kernel_lock_ready;

void nova_smp_lock_init(void) {
    if (kernel_lock_ready) return;
    uint lock_number = spin_lock_claim_unused(true);
    kernel_lock = spin_lock_instance(lock_number);
    spin_lock_init(kernel_lock);
    kernel_lock_ready = true;
}

nova_smp_lock_state_t nova_smp_lock_acquire(void) {
    if (!kernel_lock_ready) return 0u;
    return spin_lock_blocking(kernel_lock);
}

void nova_smp_lock_release(nova_smp_lock_state_t state) {
    if (kernel_lock_ready) spin_unlock(kernel_lock, state);
}
