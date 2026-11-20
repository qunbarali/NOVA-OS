#ifndef NOVA_TASK_H
#define NOVA_TASK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NOVA_TASK_MAX 8u
#define NOVA_TASK_NAME_MAX 12u
#define NOVA_TASK_PRIORITY_DEFAULT 128u

typedef uint8_t nova_task_id_t;
#define NOVA_TASK_ID_INVALID ((nova_task_id_t)0xFFu)

typedef enum {
    NOVA_TASK_UNUSED = 0,
    NOVA_TASK_READY,
    NOVA_TASK_RUNNING,
    NOVA_TASK_SLEEPING,
    NOVA_TASK_SUSPENDED,
    NOVA_TASK_FAULTED
} nova_task_state_t;

typedef enum {
    NOVA_TASK_AGAIN = 0,
    NOVA_TASK_EXIT
} nova_task_result_t;

typedef nova_task_result_t (*nova_task_entry_t)(void *context);

typedef struct {
    nova_task_id_t id;
    char name[NOVA_TASK_NAME_MAX + 1u];
    nova_task_state_t state;
    uint8_t priority;
    uint32_t runs;
    uint32_t wake_at_ms;
} nova_task_info_t;

void nova_task_init(void);
nova_task_id_t nova_task_create(const char *name, nova_task_entry_t entry, void *context);
bool nova_task_set_priority(nova_task_id_t id, uint8_t priority);
bool nova_task_suspend(nova_task_id_t id);
bool nova_task_resume(nova_task_id_t id);
bool nova_task_terminate(nova_task_id_t id);
void nova_task_sleep_current(uint32_t delay_ms);
nova_task_id_t nova_task_current_id(void);
void nova_task_scheduler_poll(uint32_t now_ms);
size_t nova_task_snapshot(nova_task_info_t *out, size_t capacity);
size_t nova_task_count(void);
uint32_t nova_task_dispatch_count(void);
uint32_t nova_task_fault_count(void);

#endif
