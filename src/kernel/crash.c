#include "nova/crash.h"
#include "hardware/structs/watchdog.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"

bool nova_crash_read(nova_crash_record_t *out) {
    if (out == NULL || watchdog_hw->scratch[0] != NOVA_CRASH_MAGIC) return false;
    out->pc = watchdog_hw->scratch[1];
    out->lr = watchdog_hw->scratch[2];
    out->xpsr = watchdog_hw->scratch[3];
    out->exc_return = watchdog_hw->scratch[4];
    out->frame_address = watchdog_hw->scratch[5];
    out->r0 = watchdog_hw->scratch[6];
    out->r1 = watchdog_hw->scratch[7];
    return true;
}

void nova_crash_clear(void) {
    for (unsigned i = 0u; i < 8u; ++i) watchdog_hw->scratch[i] = 0u;
}

void nova_arch_fault_entry(const uint32_t *frame, uint32_t exc_return) {
    /* An extended FP frame places the basic eight-word frame 18 words higher. */
    if ((exc_return & 0x10u) == 0u && frame != NULL) frame += 18;
    watchdog_hw->scratch[0] = 0u;
    watchdog_hw->scratch[1] = frame != NULL ? frame[6] : 0u;
    watchdog_hw->scratch[2] = frame != NULL ? frame[5] : 0u;
    watchdog_hw->scratch[3] = frame != NULL ? frame[7] : 0u;
    watchdog_hw->scratch[4] = exc_return;
    watchdog_hw->scratch[5] = (uint32_t)(uintptr_t)frame;
    watchdog_hw->scratch[6] = frame != NULL ? frame[0] : 0u;
    watchdog_hw->scratch[7] = frame != NULL ? frame[1] : 0u;
    __dmb();
    watchdog_hw->scratch[0] = NOVA_CRASH_MAGIC;
    watchdog_reboot(0u, 0u, 0u);
    for (;;) tight_loop_contents();
}
