#ifndef NOVA_KERNEL_H
#define NOVA_KERNEL_H
#include <stdbool.h>
#include <stdint.h>
void nova_kernel_init(void);
void nova_core1_entry(void);
void nova_kernel_poll(void);
uint32_t nova_uptime_ms(void);
uint32_t nova_system_ticks(void);
bool nova_kernel_timer_ready(void);
uint32_t nova_task_ticks(void);
bool nova_core1_is_ready(void);
#endif
