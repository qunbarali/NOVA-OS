#ifdef NOVA_HOST_TEST
#include "nova/smp_lock.h"
void nova_smp_lock_init(void) { }
nova_smp_lock_state_t nova_smp_lock_acquire(void) { return 0u; }
void nova_smp_lock_release(nova_smp_lock_state_t state) { (void)state; }
#else
#include "nova/smp_lock.h"
#include <stdbool.h>
#include "hardware/sync.h"

static spin_lock_t *kernel_lock;
static bool kernel_lock_ready;

void nova_smp_lock_init(void) {
    if (kernel_lock_ready) return;
    int claimed_lock = spin_lock_claim_unused(true);
    kernel_lock = spin_lock_init((uint)claimed_lock);
    kernel_lock_ready = true;
}

nova_smp_lock_state_t nova_smp_lock_acquire(void) {
    if (!kernel_lock_ready) return 0u;
    return spin_lock_blocking(kernel_lock);
}

void nova_smp_lock_release(nova_smp_lock_state_t state) {
    if (kernel_lock_ready) spin_unlock(kernel_lock, state);
}

#endif
