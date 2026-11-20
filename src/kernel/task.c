#include "nova/task.h"
#include <string.h>

#define NOVA_STACK_GUARD_WORD 0x4E4F5641u
#define NOVA_STACK_FILL_WORD  0xA5A5A5A5u
#define NOVA_STACK_GUARD_WORDS 4u
#define NOVA_TASK_STACK_WORDS (NOVA_TASK_STACK_SIZE / sizeof(uint32_t))

typedef struct {
    nova_task_info_t info;
    nova_task_entry_t entry;
    void *context;
    bool sleep_requested;
    uint32_t guard[NOVA_STACK_GUARD_WORDS];
    uint32_t stack[NOVA_TASK_STACK_WORDS] __attribute__((aligned(8)));
} task_control_t;

typedef struct {
    task_control_t *task;
    nova_task_result_t result;
} task_call_t;

extern void nova_arch_call_on_stack(uintptr_t stack_top, void (*function)(void *), void *argument);

static task_control_t tasks[NOVA_TASK_MAX];
static nova_task_id_t current_task = NOVA_TASK_ID_INVALID;
static uint8_t next_slot;
static uint32_t dispatches;
static uint32_t faults;
static bool scheduler_active;

static bool deadline_reached(uint32_t now, uint32_t deadline) {
    return (uint32_t)(now - deadline) < 0x80000000u;
}

static void invoke_task(void *argument) {
    task_call_t *call = (task_call_t *)argument;
    call->result = call->task->entry(call->task->context);
}

static void initialize_stack(task_control_t *task) {
    for (size_t i = 0u; i < NOVA_STACK_GUARD_WORDS; ++i) task->guard[i] = NOVA_STACK_GUARD_WORD;
    for (size_t i = 0u; i < NOVA_TASK_STACK_WORDS; ++i) task->stack[i] = NOVA_STACK_FILL_WORD;
    task->info.stack_size_bytes = (uint16_t)NOVA_TASK_STACK_SIZE;
    task->info.stack_used_bytes = 0u;
}

static bool stack_guard_valid(const task_control_t *task) {
    for (size_t i = 0u; i < NOVA_STACK_GUARD_WORDS; ++i)
        if (task->guard[i] != NOVA_STACK_GUARD_WORD) return false;
    return true;
}

static void update_stack_usage(task_control_t *task) {
    size_t touched_from_top = 0u;
    while (touched_from_top < NOVA_TASK_STACK_WORDS &&
           task->stack[NOVA_TASK_STACK_WORDS - 1u - touched_from_top] != NOVA_STACK_FILL_WORD)
        ++touched_from_top;
    task->info.stack_used_bytes = (uint16_t)(touched_from_top * sizeof(uint32_t));
}

void nova_task_init(void) {
    memset(tasks, 0, sizeof(tasks));
    current_task = NOVA_TASK_ID_INVALID;
    next_slot = 0u;
    dispatches = 0u;
    faults = 0u;
    scheduler_active = false;
}

nova_task_id_t nova_task_create(const char *name, nova_task_entry_t entry, void *context) {
    if (name == NULL || entry == NULL || name[0] == '\0') return NOVA_TASK_ID_INVALID;
    for (uint8_t n = 0u; n < NOVA_TASK_MAX; ++n) {
        uint8_t i = (uint8_t)((next_slot + n) % NOVA_TASK_MAX);
        if (tasks[i].info.state != NOVA_TASK_UNUSED) continue;
        task_control_t *task = &tasks[i];
        memset(task, 0, sizeof(*task));
        task->info.id = i;
        size_t length = strlen(name);
        if (length > NOVA_TASK_NAME_MAX) length = NOVA_TASK_NAME_MAX;
        memcpy(task->info.name, name, length);
        task->info.name[length] = '\0';
        task->info.state = NOVA_TASK_READY;
        task->info.priority = NOVA_TASK_PRIORITY_DEFAULT;
        task->entry = entry;
        task->context = context;
        initialize_stack(task);
        next_slot = (uint8_t)((i + 1u) % NOVA_TASK_MAX);
        return i;
    }
    return NOVA_TASK_ID_INVALID;
}

bool nova_task_set_priority(nova_task_id_t id, uint8_t priority) {
    if (id >= NOVA_TASK_MAX || tasks[id].info.state == NOVA_TASK_UNUSED ||
        tasks[id].info.state == NOVA_TASK_FAULTED ||
        tasks[id].info.state == NOVA_TASK_RUNNING) return false;
    tasks[id].info.priority = priority;
    return true;
}

bool nova_task_suspend(nova_task_id_t id) {
    if (id >= NOVA_TASK_MAX || tasks[id].info.state == NOVA_TASK_UNUSED ||
        tasks[id].info.state == NOVA_TASK_RUNNING || tasks[id].info.state == NOVA_TASK_FAULTED) return false;
    tasks[id].info.state = NOVA_TASK_SUSPENDED;
    return true;
}

bool nova_task_resume(nova_task_id_t id) {
    if (id >= NOVA_TASK_MAX || tasks[id].info.state != NOVA_TASK_SUSPENDED) return false;
    tasks[id].info.state = NOVA_TASK_READY;
    return true;
}

bool nova_task_terminate(nova_task_id_t id) {
    if (id >= NOVA_TASK_MAX || tasks[id].info.state == NOVA_TASK_UNUSED ||
        tasks[id].info.state == NOVA_TASK_RUNNING) return false;
    memset(&tasks[id], 0, sizeof(tasks[id]));
    return true;
}

void nova_task_sleep_current(uint32_t delay_ms) {
    if (current_task >= NOVA_TASK_MAX) return;
    task_control_t *task = &tasks[current_task];
    task->info.wake_at_ms += delay_ms;
    task->sleep_requested = true;
}

nova_task_id_t nova_task_current_id(void) { return current_task; }

void nova_task_scheduler_poll(uint32_t now_ms) {
    if (scheduler_active) return;
    scheduler_active = true;

    for (uint8_t i = 0u; i < NOVA_TASK_MAX; ++i) {
        if (tasks[i].info.state == NOVA_TASK_SLEEPING &&
            deadline_reached(now_ms, tasks[i].info.wake_at_ms)) tasks[i].info.state = NOVA_TASK_READY;
    }

    const uint8_t scan_start = next_slot;
    bool dispatched[NOVA_TASK_MAX] = { false };
    for (uint8_t n = 0u; n < NOVA_TASK_MAX; ++n) {
        uint8_t selected = NOVA_TASK_ID_INVALID;
        uint8_t best_priority = UINT8_MAX;
        for (uint8_t offset = 0u; offset < NOVA_TASK_MAX; ++offset) {
            uint8_t i = (uint8_t)((scan_start + offset) % NOVA_TASK_MAX);
            task_control_t *candidate = &tasks[i];
            if (dispatched[i] || candidate->info.state != NOVA_TASK_READY || candidate->entry == NULL) continue;
            if (selected == NOVA_TASK_ID_INVALID || candidate->info.priority < best_priority) {
                selected = i;
                best_priority = candidate->info.priority;
            }
        }
        if (selected == NOVA_TASK_ID_INVALID) break;

        dispatched[selected] = true;
        task_control_t *task = &tasks[selected];
        if (!stack_guard_valid(task)) {
            task->info.state = NOVA_TASK_FAULTED;
            task->entry = NULL;
            ++faults;
            continue;
        }

        current_task = selected;
        task->info.state = NOVA_TASK_RUNNING;
        task->info.wake_at_ms = now_ms;
        task->sleep_requested = false;
        task_call_t call = { .task = task, .result = NOVA_TASK_AGAIN };
        uintptr_t stack_top = (uintptr_t)&task->stack[NOVA_TASK_STACK_WORDS];
        nova_arch_call_on_stack(stack_top, invoke_task, &call);
        ++task->info.runs;
        ++dispatches;
        update_stack_usage(task);

        if (!stack_guard_valid(task)) {
            task->info.state = NOVA_TASK_FAULTED;
            task->entry = NULL;
            ++faults;
        } else if (call.result == NOVA_TASK_EXIT) {
            memset(task, 0, sizeof(*task));
        } else if (call.result != NOVA_TASK_AGAIN) {
            task->info.state = NOVA_TASK_FAULTED;
            task->entry = NULL;
            ++faults;
        } else if (task->sleep_requested) {
            task->info.state = NOVA_TASK_SLEEPING;
        } else {
            task->info.state = NOVA_TASK_READY;
        }
    }
    current_task = NOVA_TASK_ID_INVALID;
    next_slot = (uint8_t)((scan_start + 1u) % NOVA_TASK_MAX);
    scheduler_active = false;
}

size_t nova_task_snapshot(nova_task_info_t *out, size_t capacity) {
    if (out == NULL || capacity == 0u) return 0u;
    size_t copied = 0u;
    for (uint8_t i = 0u; i < NOVA_TASK_MAX && copied < capacity; ++i)
        if (tasks[i].info.state != NOVA_TASK_UNUSED) out[copied++] = tasks[i].info;
    return copied;
}

size_t nova_task_count(void) {
    size_t count = 0u;
    for (uint8_t i = 0u; i < NOVA_TASK_MAX; ++i)
        if (tasks[i].info.state != NOVA_TASK_UNUSED) ++count;
    return count;
}

uint32_t nova_task_dispatch_count(void) { return dispatches; }
uint32_t nova_task_fault_count(void) { return faults; }
