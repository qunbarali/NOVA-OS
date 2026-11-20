#include "nova/task.h"
#include <string.h>
#ifndef NOVA_HOST_TEST
#include "pico/multicore.h"
#include "pico/platform.h"
#endif
#define NOVA_STACK_GUARD_WORD 0x4E4F5641u
#define NOVA_STACK_FILL_WORD 0xA5A5A5A5u
#define NOVA_STACK_GUARD_WORDS 4u
#define NOVA_TASK_STACK_WORDS (NOVA_TASK_STACK_SIZE / sizeof(uint32_t))
typedef struct {
    nova_task_info_t info;
    nova_task_entry_t entry;
    void *context;
    uintptr_t saved_sp;
    nova_task_result_t result;
    bool completed;
    uint32_t guard[NOVA_STACK_GUARD_WORDS];
    uint32_t stack[NOVA_TASK_STACK_WORDS] __attribute__((aligned(8)));
} task_control_t;
#ifndef NOVA_HOST_TEST
extern void nova_arch_context_switch(uintptr_t *save_sp, uintptr_t load_sp, uintptr_t handler_sp);
static uintptr_t scheduler_sp;
#define NOVA_INTERRUPT_STACK_WORDS 256u
static uint32_t interrupt_stack[NOVA_INTERRUPT_STACK_WORDS] __attribute__((aligned(8)));
#endif
static task_control_t tasks[NOVA_TASK_MAX];
static nova_task_id_t current_task = NOVA_TASK_ID_INVALID;
static uint8_t next_slot;
static uint32_t dispatches, faults, scheduler_now_ms;
static bool scheduler_active;
static bool scheduler_core(void) {
#ifndef NOVA_HOST_TEST
    return get_core_num() == 0u;
#else
    return true;
#endif
}
static volatile uint8_t wake_pending[NOVA_TASK_MAX];
static nova_fault_record_t fault_log[NOVA_FAULT_LOG_CAPACITY];
static uint32_t fault_sequence;
static size_t fault_log_count;
static void record_task_fault(task_control_t *task, nova_fault_reason_t reason) {
    nova_fault_record_t *record = &fault_log[fault_sequence % NOVA_FAULT_LOG_CAPACITY];
    memset(record, 0, sizeof(*record));
    record->sequence = ++fault_sequence;
    record->tick_ms = scheduler_now_ms;
    record->task_id = task->info.id;
    record->reason = reason;
    memcpy(record->task_name, task->info.name, sizeof(record->task_name));
    if (fault_log_count < NOVA_FAULT_LOG_CAPACITY) ++fault_log_count;
    task->info.state = NOVA_TASK_FAULTED;
    task->entry = NULL;
    ++faults;
}
static bool deadline_reached(uint32_t now, uint32_t deadline) {
    return (uint32_t)(now - deadline) < 0x80000000u;
}
#ifndef NOVA_HOST_TEST
static void task_bootstrap(void) {
    if (current_task < NOVA_TASK_MAX) {
        task_control_t *task = &tasks[current_task];
        task->result = task->entry(task->context);
        task->completed = true;
        nova_arch_context_switch(&task->saved_sp, scheduler_sp, 0u);
    }
    for (;;) { }
}
#endif
/* Stacks grow down. Scan from the low address to find the deepest touched word. */
static void update_stack_usage(task_control_t *task) {
    size_t untouched = 0u;
    while (untouched < NOVA_TASK_STACK_WORDS &&
           task->stack[untouched] == NOVA_STACK_FILL_WORD) {
        ++untouched;
    }
    size_t used = NOVA_TASK_STACK_WORDS - untouched;
    task->info.stack_used_bytes = (uint16_t)(used * sizeof(uint32_t));
}
static void initialize_stack(task_control_t *task) {
    for (size_t i = 0u; i < NOVA_STACK_GUARD_WORDS; ++i) task->guard[i] = NOVA_STACK_GUARD_WORD;
    for (size_t i = 0u; i < NOVA_TASK_STACK_WORDS; ++i) task->stack[i] = NOVA_STACK_FILL_WORD;
    uintptr_t *sp = (uintptr_t *)&task->stack[NOVA_TASK_STACK_WORDS];
    uintptr_t entry_pc = 0u;
#ifndef NOVA_HOST_TEST
    entry_pc = (uintptr_t)(void (*)(void))task_bootstrap;
#endif
    *--sp = entry_pc;
#if defined(NOVA_ARCH_ARMV6M) || defined(NOVA_ARCH_ARMV8M)
    for (size_t i = 0u; i < 8u; ++i) *--sp = 0u;
#elif !defined(NOVA_HOST_TEST)
#error "Select a NOVA Cortex-M architecture port"
#endif
    task->saved_sp = (uintptr_t)sp;
    task->completed = false;
    task->result = NOVA_TASK_AGAIN;
    task->info.stack_size_bytes = (uint16_t)NOVA_TASK_STACK_SIZE;
    update_stack_usage(task);
}
static bool stack_guard_valid(const task_control_t *task) {
    for (size_t i = 0u; i < NOVA_STACK_GUARD_WORDS; ++i)
        if (task->guard[i] != NOVA_STACK_GUARD_WORD) return false;
    return true;
}
void nova_task_init(void) {
    if (!scheduler_core()) return;
    memset(tasks, 0, sizeof(tasks)); current_task = NOVA_TASK_ID_INVALID; next_slot = 0u;
    dispatches = 0u; faults = 0u; scheduler_now_ms = 0u; scheduler_active = false;
    memset(fault_log, 0, sizeof(fault_log)); fault_sequence = 0u; fault_log_count = 0u;
    memset((void *)wake_pending, 0, sizeof(wake_pending));
#ifndef NOVA_HOST_TEST
    scheduler_sp = 0u;
#endif
}
nova_task_id_t nova_task_create(const char *name, nova_task_entry_t entry, void *context) {
    if (!scheduler_core() || name == NULL || entry == NULL || name[0] == '\0') return NOVA_TASK_ID_INVALID;
    for (uint8_t n = 0u; n < NOVA_TASK_MAX; ++n) {
        uint8_t i = (uint8_t)((next_slot + n) % NOVA_TASK_MAX);
        if (tasks[i].info.state != NOVA_TASK_UNUSED) continue;
        task_control_t *task = &tasks[i]; memset(task, 0, sizeof(*task)); task->info.id = i;
        size_t length = strlen(name); if (length > NOVA_TASK_NAME_MAX) length = NOVA_TASK_NAME_MAX;
        memcpy(task->info.name, name, length); task->info.name[length] = '\0';
        task->info.state = NOVA_TASK_READY; task->info.priority = NOVA_TASK_PRIORITY_DEFAULT;
        task->entry = entry; task->context = context; __atomic_store_n(&wake_pending[i], (uint8_t)0u, __ATOMIC_RELAXED); initialize_stack(task);
        next_slot = (uint8_t)((i + 1u) % NOVA_TASK_MAX); return i;
    }
    return NOVA_TASK_ID_INVALID;
}
bool nova_task_set_priority(nova_task_id_t id, uint8_t priority) {
    if (!scheduler_core() || id >= NOVA_TASK_MAX || tasks[id].info.state == NOVA_TASK_UNUSED ||
        tasks[id].info.state == NOVA_TASK_FAULTED || tasks[id].info.state == NOVA_TASK_RUNNING) return false;
    tasks[id].info.priority = priority; return true;
}
bool nova_task_suspend(nova_task_id_t id) {
    if (!scheduler_core() || id >= NOVA_TASK_MAX || tasks[id].info.state == NOVA_TASK_UNUSED ||
        tasks[id].info.state == NOVA_TASK_RUNNING || tasks[id].info.state == NOVA_TASK_FAULTED) return false;
    tasks[id].info.state = NOVA_TASK_SUSPENDED; return true;
}
bool nova_task_resume(nova_task_id_t id) {
    if (!scheduler_core() || id >= NOVA_TASK_MAX || tasks[id].info.state != NOVA_TASK_SUSPENDED) return false;
    tasks[id].info.state = NOVA_TASK_READY; return true;
}
bool nova_task_terminate(nova_task_id_t id) {
    if (!scheduler_core() || id >= NOVA_TASK_MAX || tasks[id].info.state == NOVA_TASK_UNUSED ||
        tasks[id].info.state == NOVA_TASK_RUNNING) return false;
    memset(&tasks[id], 0, sizeof(tasks[id])); __atomic_store_n(&wake_pending[id], (uint8_t)0u, __ATOMIC_RELAXED); return true;
}
static void switch_current_to_scheduler(void) {
    if (current_task >= NOVA_TASK_MAX) return;
#ifndef NOVA_HOST_TEST
    task_control_t *task = &tasks[current_task];
    nova_arch_context_switch(&task->saved_sp, scheduler_sp, 0u);
#endif
}
void nova_task_yield(void) {
    if (nova_task_current_id() == NOVA_TASK_ID_INVALID) return;
    task_control_t *task = &tasks[current_task];
    if (task->info.state != NOVA_TASK_RUNNING) return;
    task->info.state = NOVA_TASK_READY; switch_current_to_scheduler();
}
bool nova_task_prepare_block(uint32_t timeout_ms) {
    if (nova_task_current_id() == NOVA_TASK_ID_INVALID) return false;
    task_control_t *task = &tasks[current_task];
    if (task->info.state != NOVA_TASK_RUNNING) return false;
    __atomic_store_n(&wake_pending[current_task], (uint8_t)0u, __ATOMIC_RELAXED);
    task->info.wake_at_ms = timeout_ms == 0u ? 0u : scheduler_now_ms + timeout_ms;
    task->info.state = NOVA_TASK_BLOCKED;
    return true;
}
void nova_task_commit_block(void) {
    if (nova_task_current_id() != NOVA_TASK_ID_INVALID && tasks[current_task].info.state == NOVA_TASK_BLOCKED)
        switch_current_to_scheduler();
}
bool nova_task_wake(nova_task_id_t id) {
    if (id >= NOVA_TASK_MAX) return false;
    __atomic_store_n(&wake_pending[id], (uint8_t)1u, __ATOMIC_RELEASE);
    return true;
}
uint32_t nova_task_now_ms(void) { return scheduler_now_ms; }

void nova_task_sleep_current(uint32_t delay_ms) {
    if (nova_task_current_id() == NOVA_TASK_ID_INVALID) return;
    task_control_t *task = &tasks[current_task];
    if (task->info.state != NOVA_TASK_RUNNING) return;
    task->info.wake_at_ms = scheduler_now_ms + delay_ms;
    task->info.state = NOVA_TASK_SLEEPING; switch_current_to_scheduler();
}
nova_task_id_t nova_task_current_id(void) { return scheduler_core() ? current_task : NOVA_TASK_ID_INVALID; }
void nova_task_scheduler_poll(uint32_t now_ms) {
    if (!scheduler_core() || scheduler_active) return;
    scheduler_active = true; scheduler_now_ms = now_ms;
    for (uint8_t i = 0u; i < NOVA_TASK_MAX; ++i) {
        if (__atomic_exchange_n(&wake_pending[i], (uint8_t)0u, __ATOMIC_ACQ_REL) != 0u &&
            tasks[i].info.state == NOVA_TASK_BLOCKED) {
            tasks[i].info.state = NOVA_TASK_READY;
            tasks[i].info.wake_at_ms = 0u;
        }
        if ((tasks[i].info.state == NOVA_TASK_SLEEPING ||
             (tasks[i].info.state == NOVA_TASK_BLOCKED && tasks[i].info.wake_at_ms != 0u)) &&
            deadline_reached(now_ms, tasks[i].info.wake_at_ms)) {
            tasks[i].info.state = NOVA_TASK_READY;
            tasks[i].info.wake_at_ms = 0u;
        }
    }
    const uint8_t scan_start = next_slot;
    bool dispatched[NOVA_TASK_MAX] = {false};
    for (uint8_t n = 0u; n < NOVA_TASK_MAX; ++n) {
        uint8_t selected = NOVA_TASK_ID_INVALID, best_priority = UINT8_MAX;
        for (uint8_t offset = 0u; offset < NOVA_TASK_MAX; ++offset) {
            uint8_t i = (uint8_t)((scan_start + offset) % NOVA_TASK_MAX);
            task_control_t *candidate = &tasks[i];
            if (dispatched[i] || candidate->info.state != NOVA_TASK_READY || candidate->entry == NULL) continue;
            if (selected == NOVA_TASK_ID_INVALID || candidate->info.priority < best_priority) {
                selected = i; best_priority = candidate->info.priority;
            }
        }
        if (selected == NOVA_TASK_ID_INVALID) break;
        dispatched[selected] = true;
        task_control_t *task = &tasks[selected];
        if (!stack_guard_valid(task)) {
            record_task_fault(task, NOVA_FAULT_STACK_GUARD); continue;
        }
        current_task = selected; task->info.state = NOVA_TASK_RUNNING; ++task->info.runs; ++dispatches;
#ifdef NOVA_HOST_TEST
        task->result = task->entry(task->context); task->completed = true;
#else
        nova_arch_context_switch(&scheduler_sp, task->saved_sp,
                                 (uintptr_t)&interrupt_stack[NOVA_INTERRUPT_STACK_WORDS]);
#endif
        current_task = NOVA_TASK_ID_INVALID; update_stack_usage(task);
        if (!stack_guard_valid(task)) {
            record_task_fault(task, NOVA_FAULT_STACK_GUARD);
        } else if (task->info.stack_used_bytes >= NOVA_TASK_STACK_LIMIT_BYTES) {
            record_task_fault(task, NOVA_FAULT_STACK_LIMIT);
        } else if (task->completed) {
            if (task->result == NOVA_TASK_EXIT) memset(task, 0, sizeof(*task));
            else if (task->result != NOVA_TASK_AGAIN) {
                record_task_fault(task, NOVA_FAULT_INVALID_TASK_RESULT);
            } else if (task->info.state == NOVA_TASK_RUNNING || task->info.state == NOVA_TASK_READY) {
                initialize_stack(task); task->info.state = NOVA_TASK_READY;
            }
        } else if (task->info.state == NOVA_TASK_RUNNING) {
            record_task_fault(task, NOVA_FAULT_TASK_RETURNED_RUNNING);
        }
    }
    current_task = NOVA_TASK_ID_INVALID;
    next_slot = (uint8_t)((scan_start + 1u) % NOVA_TASK_MAX); scheduler_active = false;
}
size_t nova_task_snapshot(nova_task_info_t *out, size_t capacity) {
    if (!scheduler_core() || out == NULL || capacity == 0u) return 0u;
    size_t copied = 0u;
    for (uint8_t i = 0u; i < NOVA_TASK_MAX && copied < capacity; ++i)
        if (tasks[i].info.state != NOVA_TASK_UNUSED) out[copied++] = tasks[i].info;
    return copied;
}
size_t nova_task_count(void) {
    if (!scheduler_core()) return 0u;
    size_t count = 0u;
    for (uint8_t i = 0u; i < NOVA_TASK_MAX; ++i) if (tasks[i].info.state != NOVA_TASK_UNUSED) ++count;
    return count;
}
uint32_t nova_task_dispatch_count(void) { return scheduler_core() ? dispatches : 0u; }
uint32_t nova_task_fault_count(void) { return scheduler_core() ? faults : 0u; }
size_t nova_task_fault_snapshot(nova_fault_record_t *out, size_t capacity) {
    if (!scheduler_core() || out == NULL || capacity == 0u) return 0u;
    size_t count = fault_log_count < capacity ? fault_log_count : capacity;
    uint32_t first = fault_sequence - (uint32_t)fault_log_count;
    if (fault_log_count > count) first += (uint32_t)(fault_log_count - count);
    for (size_t i = 0u; i < count; ++i) {
        uint32_t sequence = first + (uint32_t)i + 1u;
        out[i] = fault_log[(sequence - 1u) % NOVA_FAULT_LOG_CAPACITY];
    }
    return count;
}
