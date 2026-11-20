#ifndef NOVA_CRITICAL_H
#define NOVA_CRITICAL_H
#include <stdint.h>

/*
 * Core-local interrupt masking for short critical sections.
 * This is not an SMP lock: it does not exclude the other RP core.
 * Always restore the exact state returned by enter().
 */
typedef uint32_t nova_irq_state_t;
nova_irq_state_t nova_critical_enter(void);
void nova_critical_exit(nova_irq_state_t state);
#endif
