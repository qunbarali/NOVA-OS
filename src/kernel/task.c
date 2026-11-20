#include "nova/task.h"
#include <string.h>

typedef struct {
    nova_task_info_t info;
    nova_task_entry_t entry;
    void *context;
    bool sleep_requested;
} task_control_t;

static task_control_t tasks[NOVA_TASK_MAX];
static nova_task_id_t current_task = NOVA_TASK_ID_INVALID;
static uint8_t next_slot;
static uint32_t dispatches;

static bool deadline_reached(uint32_t now, uint32_t deadline) {
    return (int32_t)(now - deadline) >= 0;
}

void nova_task_init(void) {
    memset(tasks, 0, sizeof(tasks));
    current_task = NOVA_TASK_ID_INVALID;
    next_slot = 0u;
    dispatches = 0u;
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
        task->entry = entry;
        task->context = context;
        next_slot = (uint8_t)((i + 1u) % NOVA_TASK_MAX);
        return i;
    }
    return NOVA_TASK_ID_INVALID;
}

bool nova_task_suspend(nova_task_id_t id) {
    if (id >= NOVA_TASK_MAX || tasks[id].info.state == NOVA_TASK_UNUSED ||
        tasks[id].info.state == NOVA_TASK_RUNNING) return false;
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
    task->info.wake_at_ms = task->info.wake_at_ms + delay_ms;
    /* The scheduler sets wake_at_ms to the current time before dispatch. */
    task->sleep_requested = true;
}

void nova_task_yield_current(void) {
    if (current_task < NOVA_TASK_MAX) tasks[current_task].sleep_requested = false;
}

void nova_task_scheduler_poll(uint32_t now_ms) {
    for (uint8_t i = 0u; i < NOVA_TASK_MAX; ++i) {
        if (tasks[i].info.state == NOVA_TASK_SLEEPING &&
            deadline_reached(now_ms, tasks[i].info.wake_at_ms)) {
            tasks[i].info.state = NOVA_TASK_READY;
        }
    }

    for (uint8_t n = 0u; n < NOVA_TASK_MAX; ++n) {
        uint8_t i = (uint8_t)((next_slot + n) % NOVA_TASK_MAX);
        task_control_t *task = &tasks[i];
        if (task->info.state != NOVA_TASK_READY || task->entry == NULL) continue;

        current_task = i;
        task->info.state = NOVA_TASK_RUNNING;
        task->info.wake_at_ms = now_ms;
        task->sleep_requested = false;
        nova_task_result_t result = task->entry(task->context);
        ++task->info.runs;
        ++dispatches;

        if (result == NOVA_TASK_EXIT) {
            memset(task, 0, sizeof(*task));
        } else if (task->sleep_requested) {
            task->info.state = NOVA_TASK_SLEEPING;
        } else {
            task->info.state = NOVA_TASK_READY;
        }
        next_slot = (uint8_t)((i + 1u) % NOVA_TASK_MAX);
    }
    current_task = NOVA_TASK_ID_INVALID;
}

size_t nova_task_snapshot(nova_task_info_t *out, size_t capacity) {
    if (out == NULL || capacity == 0u) return 0u;
    size_t copied = 0u;
    for (uint8_t i = 0u; i < NOVA_TASK_MAX && copied < capacity; ++i) {
        if (tasks[i].info.state == NOVA_TASK_UNUSED) continue;
        out[copied++] = tasks[i].info;
    }
    return copied;
}

size_t nova_task_count(void) {
    size_t count = 0u;
    for (uint8_t i = 0u; i < NOVA_TASK_MAX; ++i)
        if (tasks[i].info.state != NOVA_TASK_UNUSED) ++count;
    return count;
}

uint32_t nova_task_dispatch_count(void) { return dispatches; }
