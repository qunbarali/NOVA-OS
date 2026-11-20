#ifndef NOVA_SMP_LOCK_H
#define NOVA_SMP_LOCK_H
#include <stdint.h>
typedef uint32_t nova_smp_lock_state_t;
/* Initialize once before starting core 1. These locks are not ISR-reentrant. */
void nova_smp_lock_init(void);
nova_smp_lock_state_t nova_smp_lock_acquire(void);
void nova_smp_lock_release(nova_smp_lock_state_t state);
#endif
