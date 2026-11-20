#ifndef NOVA_CRASH_H
#define NOVA_CRASH_H
#include <stdbool.h>
#include <stdint.h>
#define NOVA_CRASH_MAGIC 0x4E564352u
typedef struct {
    uint32_t pc;
    uint32_t lr;
    uint32_t xpsr;
    uint32_t exc_return;
    uint32_t frame_address;
    uint32_t r0;
    uint32_t r1;
} nova_crash_record_t;
/* Returns true when a watchdog-retained Cortex-M fault record is available. */
bool nova_crash_read(nova_crash_record_t *out);
void nova_crash_clear(void);
/* Architecture exception entry; never returns. */
void nova_arch_fault_entry(const uint32_t *stacked_frame, uint32_t exc_return)
    __attribute__((noreturn));
#endif
