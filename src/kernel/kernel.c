#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "nova/kernel.h"

static absolute_time_t boot_time;
static volatile uint32_t core1_heartbeat;
static volatile bool core1_ready;

void nova_kernel_init(void) {
    boot_time = get_absolute_time();
    __atomic_store_n(&core1_heartbeat, 0u, __ATOMIC_RELAXED);
    __atomic_store_n(&core1_ready, false, __ATOMIC_RELEASE);
}

void nova_core1_entry(void) {
    __atomic_store_n(&core1_ready, true, __ATOMIC_RELEASE);
    for (;;) {
        __atomic_add_fetch(&core1_heartbeat, 1u, __ATOMIC_RELAXED);
        tight_loop_contents();
    }
}

uint32_t nova_uptime_ms(void) {
    return (uint32_t)(absolute_time_diff_us(boot_time, get_absolute_time()) / 1000);
}

uint32_t nova_task_ticks(void) {
    return __atomic_load_n(&core1_heartbeat, __ATOMIC_RELAXED);
}

bool nova_core1_is_ready(void) {
    return __atomic_load_n(&core1_ready, __ATOMIC_ACQUIRE);
}
