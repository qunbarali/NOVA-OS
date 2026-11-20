#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "nova/kernel.h"
#include "nova/task.h"
#include "nova/memory.h"
#include "nova/smp_lock.h"
#include "nova/device.h"

static absolute_time_t boot_time;
static repeating_timer_t system_timer;
static volatile uint32_t system_ticks;
static volatile uint32_t core1_heartbeat;
static volatile bool core1_ready;
static bool timer_ready;

static bool nova_tick_callback(repeating_timer_t *timer) {
    (void)timer;
    __atomic_add_fetch(&system_ticks, 1u, __ATOMIC_RELAXED);
    return true;
}

void nova_kernel_init(void) {
    nova_smp_lock_init();
    nova_device_manager_init();
    boot_time = get_absolute_time();
    __atomic_store_n(&system_ticks, 0u, __ATOMIC_RELAXED);
    __atomic_store_n(&core1_heartbeat, 0u, __ATOMIC_RELAXED);
    __atomic_store_n(&core1_ready, false, __ATOMIC_RELEASE);
    timer_ready = add_repeating_timer_ms(-1, nova_tick_callback, NULL, &system_timer);
    nova_task_init();
    nova_memory_init();
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

uint32_t nova_system_ticks(void) {
    return __atomic_load_n(&system_ticks, __ATOMIC_RELAXED);
}

bool nova_kernel_timer_ready(void) { return timer_ready; }

void nova_kernel_poll(void) { nova_task_scheduler_poll(nova_uptime_ms()); }
uint32_t nova_task_ticks(void) { return __atomic_load_n(&core1_heartbeat, __ATOMIC_RELAXED); }
bool nova_core1_is_ready(void) { return __atomic_load_n(&core1_ready, __ATOMIC_ACQUIRE); }
